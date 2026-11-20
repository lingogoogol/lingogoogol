#!/usr/bin/env python3
"""Exercise desktop OAuth against local fixtures, never real Google credentials.

Wine lacks CNG ECDSA/OAEP. The protocol tests substitute only a software test KSP
and browser launcher. A separate executable checks the unmodified failure path.
"""
import base64
import hashlib
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
import json
import os
import re
from pathlib import Path
import shlex
import shutil
import struct
import subprocess
import tempfile
import threading
import time
import traceback
import urllib.error
import urllib.parse
import urllib.request
import zlib

source = Path(__file__).resolve().parents[1]
build = source.parent / 'build/x64-win32-pe-mingw_Release'
runtime = source.parent / 'install/x64-win32-pe-mingw_Release'


def b64(data):
    return base64.urlsafe_b64encode(data).decode().rstrip('=')


def unb64(text):
    return base64.urlsafe_b64decode(text + '=' * (-len(text) % 4))


def png():
    def chunk(kind, data):
        return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))
    rows = b''.join(b'\0' + b''.join(bytes((240, 40, 30, 255) if x < 16 else (30, 210, 70, 255))
                                     for x in range(32)) for y in range(32))
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', 32, 32, 8, 6, 0, 0, 0)) + \
        chunk(b'IDAT', zlib.compress(rows)) + chunk(b'IEND', b'')


class Handler(BaseHTTPRequestHandler):
    def log_message(self, *unused):
        pass

    def reply(self, status, data, nonce=None):
        if not isinstance(data, bytes):
            data = json.dumps(data).encode()
        self.send_response(status)
        self.send_header('Content-Length', str(len(data)))
        if nonce:
            self.send_header('DPoP-Nonce', nonce)
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self):
        if self.path.startswith('/fixture/authorize?'):
            url = urllib.parse.parse_qs(urllib.parse.urlsplit(self.path).query)['url'][0]
            query = urllib.parse.parse_qs(urllib.parse.urlsplit(url).query)
            assert query['scope'] == [cloud_scopes]
            self.server.scope = query['scope'][0]
            self.server.challenge = query['code_challenge'][0]
            self.server.redirect = query['redirect_uri'][0]
            return self.reply(200, {})
        if self.path.startswith(('/resource/', '/services/', '/iam/')):
            return self.cloud('GET')
        if self.path == '/oauth2/v2/userinfo':
            return self.reply(200, {'email': self.server.email, 'verified_email': True,
                                    'picture': self.server.host + '/photo'})
        if urllib.parse.urlsplit(self.path).path == '/drive/v3/files':
            try:
                query = urllib.parse.parse_qs(urllib.parse.urlsplit(self.path).query)
                assert query['spaces'] == ['appDataFolder'], 'Clear enumerated outside appDataFolder.'
                return self.reply(200, {'files': [{'id': 'appdata-a'}, {'id': 'appdata-b'}]})
            except Exception:
                self.server.errors.append(traceback.format_exc())
                return self.reply(400, {})
        if self.path.startswith('/drive/v3/about?'):
            assert self.headers.get('Authorization', '').startswith('Bearer fixture-access-')
            assert urllib.parse.parse_qs(urllib.parse.urlsplit(self.path).query)['fields'] == ['user(emailAddress,photoLink)']
            return self.reply(200, {'user': {'emailAddress': getattr(self.server, 'email', 'oauth@example.invalid'),
                                           'photoLink': self.server.host + '/photo'}})
        if self.path == '/photo':
            self.server.photos = getattr(self.server, 'photos', 0) + 1
            assert not self.headers.get('Authorization'), 'Credentials leaked to the picture URL.'
            return self.reply(200, png())
        # Production directory listing, redirected only by mock_google_easy.cpp.
        return self.reply(200, {'name': '{"next":"null"}', 'description': '[]'})

    def do_POST(self):
        try:
            if self.headers.get('Transfer-Encoding') == 'chunked':
                data = b''
                while True:
                    count = int(self.rfile.readline().split(b';')[0].strip(), 16)
                    if not count:
                        self.rfile.readline()
                        break
                    data += self.rfile.read(count)
                    self.rfile.read(2)
            else:
                data = self.rfile.read(int(self.headers.get('Content-Length', 0)))
            if self.path.startswith(('/resource/', '/services/', '/iam/')):
                return self.cloud('POST', data)
            if self.path.startswith('/drive/v3/files'):
                value = json.loads(data)
                assert value['parents'] == ['appDataFolder'], 'Blob escaped appDataFolder.'
                assert self.headers.get('Authorization', '').startswith('Bearer fixture-access-')
                self.server.created += 1
                return self.reply(200, {'id': 'created-' + str(self.server.created)})
            if self.path.startswith('/batch/'):
                assert self.headers.get('Authorization', '').startswith('Bearer fixture-access-')
                parts = re.findall(rb'Content-ID: (\d+)\r\n\r\n(.*?)(?=\r\n--)', data, re.S)
                assert parts
                response = []
                for identifier, part in parts:
                    first, _, body = part.partition(b'\r\n\r\n')
                    method, path, *_ = first.split(b'\r\n')[0].decode().split()
                    if method == 'POST':
                        assert b'Content-Type: application/json; charset=UTF-8\r\n' in first + b'\r\n', 'Reserve JSON Content-Type was lost.'
                        assert b'Host: www.googleapis.com\r\n' in first + b'\r\n', 'Invalid reserve Host header.'
                        assert json.loads(body)['parents'] == ['appDataFolder'], 'Reserve blob escaped appDataFolder.'
                        self.server.created += 1
                        status, value = 200, {'id': 'created-' + str(self.server.created)}
                    else:
                        assert method == 'DELETE' and path.rsplit('/', 1)[-1] in ('appdata-a', 'appdata-b')
                        self.server.deleted.add(path.rsplit('/', 1)[-1])
                        status, value = 204, None
                    response.append(b'--fixture\r\nContent-Type: application/http\r\nContent-ID: response-' + identifier +
                                    f'\r\n\r\nHTTP/1.1 {status} OK\r\n\r\n'.encode() +
                                    (json.dumps(value).encode() if value else b'') + b'\r\n')
                self.send_response(200)
                output = b''.join(response) + b'--fixture--\r\n'
                self.send_header('Content-Type', 'multipart/mixed; boundary=fixture')
                self.send_header('Content-Length', str(len(output)))
                self.end_headers()
                self.wfile.write(output)
                return
            form = urllib.parse.parse_qs(data.decode())
            assert form['client_id'] == ['fixture.apps.googleusercontent.com']
            assert form['client_secret'] == ['fixture-public-client-secret']
            token = self.headers['DPoP'].split('.')
            header, claims = [json.loads(unb64(part)) for part in token[:2]]
            assert header['typ'] == 'dpop+jwt' and header['alg'] == 'ES256'
            jwk = header['jwk']
            assert jwk['kty'] == 'EC' and jwk['crv'] == 'P-256' and 'd' not in jwk
            assert claims['htm'] == 'POST' and claims['htu'] == self.server.host + '/token'
            assert abs(claims['iat'] - time.time()) < 30
            signature = unb64(token[2])
            assert len(signature) == 64
            def integer(value):
                value = value.lstrip(b'\0') or b'\0'
                if value[0] & 128:
                    value = b'\0' + value
                return b'\x02' + bytes((len(value),)) + value
            der = integer(signature[:32]) + integer(signature[32:])
            der = b'\x30' + bytes((len(der),)) + der
            path = self.server.directory
            (path / 'verify-public.der').write_bytes(bytes.fromhex('3059301306072a8648ce3d020106082a8648ce3d03010703420004') +
                                                     unb64(jwk['x']) + unb64(jwk['y']))
            (path / 'verify-signature.der').write_bytes(der)
            (path / 'verify-data').write_bytes('.'.join(token[:2]).encode())
            subprocess.run(['openssl', 'dgst', '-sha256', '-keyform', 'DER', '-verify', str(path / 'verify-public.der'),
                            '-signature', str(path / 'verify-signature.der'), str(path / 'verify-data')],
                           check=True, capture_output=True)
            grant = form['grant_type'][0]
            if grant == 'authorization_code':
                assert form['code'] == ['fixture-code']
                assert claims['jti'] == b64(hashlib.sha256(b'fixture-code').digest())
                assert b64(hashlib.sha256(form['code_verifier'][0].encode()).digest()) == self.server.challenge
                assert form['redirect_uri'] == [self.server.redirect]
                self.server.key = jwk
                if not claims.get('nonce'):
                    return self.reply(400, {'error': 'use_dpop_nonce'}, 'code-nonce')
                assert claims['nonce'] == 'code-nonce'
            else:
                assert grant == 'refresh_token' and jwk == self.server.grants[form['refresh_token'][0]]
                assert claims['jti'] not in self.server.identifiers, 'Refresh DPoP jti was reused.'
                self.server.identifiers.add(claims['jti'])
                if claims.get('nonce') != 'refresh-challenge':
                    return self.reply(400, {'error': 'use_dpop_nonce'}, 'refresh-challenge')
                self.server.refreshes += 1
            self.server.issued += 1
            self.server.refresh = 'fixture-refresh-' + str(self.server.issued)
            self.server.grants[self.server.refresh] = jwk
            return self.reply(200, {'access_token': 'fixture-access-' + str(self.server.issued),
                                    'refresh_token': self.server.refresh, 'token_type': 'Bearer', 'expires_in': 3600,
                                    'scope': self.server.scope if not getattr(self.server, 'deny_scope', False) else
                                        'https://www.googleapis.com/auth/cloud-platform'}, 'success-nonce')
        except Exception:
            self.server.errors.append(traceback.format_exc())
            self.reply(500, {'error': 'fixture-validation'})

    def do_DELETE(self):
        self.cloud('DELETE')

    def cloud(self, method, body=b''):
        try:
            assert self.headers.get('Authorization', '').startswith('Bearer fixture-access-')
            path = urllib.parse.urlsplit(self.path).path
            value = json.loads(body) if body else {}
            self.server.cloud_calls.append((method, path))
            if path == '/resource/v3/projects:search':
                if self.server.cloud_rejections == 0:
                    self.server.cloud_rejections += 1
                    return self.reply(401, {})
                # Ignore case-insensitive/partial matches and inactive projects.
                query = urllib.parse.parse_qs(urllib.parse.urlsplit(self.path).query)
                if 'pageToken' not in query:
                    return self.reply(200, {'projects': [{'projectId': 'unrelated-project', 'displayName': 'Storage-AI', 'state': 'ACTIVE'},
                        {'projectId': 'deleted-project', 'displayName': 'storage-ai', 'state': 'DELETE_REQUESTED'}], 'nextPageToken': 'next-page'})
                assert query['pageToken'] == ['next-page']
                # Newly created projects may remain absent from search even
                # though a direct GET already sees them. Keep hiding them until
                # a specific cold-start discovery scenario opts into visibility.
                return self.reply(200, {'projects': list(self.server.projects.values()) if self.server.search_visible else []})
            if path == '/resource/v3/projects' and method == 'POST':
                assert value['displayName'] == 'storage-ai' and re.fullmatch(r'storage-ai-[a-f0-9]{16}', value['projectId'])
                if value['projectId'] in self.server.projects:
                    return self.reply(409, {})
                self.server.projects[value['projectId']] = {**value, 'name': 'projects/123456', 'state': 'ACTIVE'}
                return self.reply(200, {'name': 'operations/project-create'})
            if path.startswith('/resource/v3/projects/'):
                if self.server.project_read_status:
                    return self.reply(self.server.project_read_status, {})
                identifier = path.rsplit('/', 1)[-1]
                return self.reply(200, self.server.projects[identifier]) if identifier in self.server.projects else self.reply(404, {})
            if path == '/resource/v3/operations/project-create':
                if self.server.project_operation_failure:
                    self.server.project_operation_failure = False
                    return self.reply(503, {})
                return self.reply(200, {'done': True})
            if path == '/services/v1/projects/123456/services:batchEnable':
                assert value['serviceIds'] == ['drive.googleapis.com']
                return self.reply(200, {'name': 'operations/enable-apis'})
            if path == '/services/v1/operations/enable-apis':
                return self.reply(200, {'done': True})
            if path.startswith('/iam/v1/projects/'):
                parts = path.split('/')
                project = parts[4]
                if path.endswith('/serviceAccounts'):
                    name = value['accountId']
                    email = name + '@' + project + '.iam.gserviceaccount.com'
                    if email in self.server.service_accounts:
                        return self.reply(409, {})
                    self.server.service_accounts[email] = name
                    return self.reply(200, {'email': email})
                if path.endswith('/keys'):
                    email = urllib.parse.unquote(parts[6])
                    name = self.server.service_accounts[email]
                    assert value == {'privateKeyType': 'TYPE_GOOGLE_CREDENTIALS_FILE', 'keyAlgorithm': 'KEY_ALG_RSA_2048'}
                    if name == 'fail-key':
                        return self.reply(403, {'error': {'details': [{'reason': 'SERVICE_DISABLED',
                            'metadata': {'consumer': 'projects/999999', 'service': 'iam.googleapis.com'}}]}})
                    credential = {'type': 'service_account', 'project_id': project, 'client_email': email,
                        'private_key_id': 'fixture-key-id', 'private_key': 'fixture-cloud-private-key'}
                    if name == 'race-save':
                        (self.server.directory/'key'/f'{project}-{name}.json').write_text('keep-existing')
                    return self.reply(200, {'name': f'projects/{project}/serviceAccounts/{email}/keys/fixture-key-id',
                        'privateKeyData': base64.b64encode(json.dumps(credential).encode()).decode()})
                if method == 'DELETE':
                    self.server.revoked_keys += 1
                    return self.reply(200, {})
            raise AssertionError((method, path))
        except Exception:
            self.server.errors.append(traceback.format_exc())
            self.reply(500, {})


def compile_source(original, replacement, output):
    entries = json.loads((build / 'compile_commands.json').read_text())
    entry = next(item for item in entries if Path(item['file']) == source / original)
    command = shlex.split(entry['command'])
    command[command.index('-o') + 1] = str(output)
    command[command.index('-c') + 1] = str(replacement)
    subprocess.run(command, cwd=build, check=True)


def link(output, entry, oauth=None, easy=None, gui=False, cloud=None):
    text = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
    command = shlex.split(text.split(' && ')[1])
    if not gui:
        command = [arg for arg in command if arg not in ('-mwindows', '-municode') and not arg.startswith('-Wl,--out-implib')]
    command[command.index('-o') + 1] = str(output)
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(entry)
    for target, obj in [('oauth', oauth), ('easy', easy), ('cloud', cloud)]:
        if obj:
            command[command.index('storage/google/' + target + '/libstorage.google.' + target + '.a')] = str(obj)
            command.append('storage/google/' + target + '/CMakeFiles/storage.google.' + target + '.dir/.cppm.obj')
    subprocess.run(command, cwd=build, check=True)


cloud_scopes = 'https://www.googleapis.com/auth/cloud-platform https://www.googleapis.com/auth/userinfo.email https://www.googleapis.com/auth/userinfo.profile'

with tempfile.TemporaryDirectory(prefix='storage-oauth-') as directory:
    test = Path(directory)
    server = ThreadingHTTPServer(('127.0.0.1', 0), Handler)
    server.host = 'http://127.0.0.1:' + str(server.server_port)
    server.directory = test
    server.issued = server.refreshes = 0
    server.created = 0
    server.deleted = set()
    server.grants = {}
    server.identifiers = set()
    server.errors = []
    server.scope = 'https://www.googleapis.com/auth/drive.appdata'
    server.projects, server.service_accounts = {}, {}
    server.cloud_calls, server.revoked_keys, server.cloud_rejections = [], 0, 0
    server.search_visible, server.project_operation_failure, server.project_read_status = False, True, 0
    threading.Thread(target=server.serve_forever, daemon=True).start()
    config = {'installed': {'client_id': 'fixture.apps.googleusercontent.com',
                           'client_secret': 'fixture-public-client-secret',
                           'token_uri': server.host + '/token', 'auth_uri': server.host + '/auth'}}
    (test / 'client.json').write_text(json.dumps(config))
    original = (source / 'storage/google/oauth/.cpp').read_text()
    fixture = original.replace('module storage.google.oauth;', '#include "' + str(source / 'tests/mock_oauth_cng.hpp') +
                               '"\nmodule storage.google.oauth;')
    fixture = fixture.replace('https://oauth2.googleapis.com/token', server.host + '/token')
    fixture = fixture.replace('https://accounts.google.com/o/oauth2/v2/auth', server.host + '/auth')
    fixture = fixture.replace('https://www.googleapis.com/drive/v3/about?', server.host + '/drive/v3/about?')
    fixture = fixture.replace('https://www.googleapis.com/oauth2/v2/userinfo', server.host + '/oauth2/v2/userinfo')
    cloud_fixture = (source/'storage/google/cloud/.cpp').read_text()
    for host, route in [('cloudresourcemanager', 'resource'), ('serviceusage', 'services'), ('iam', 'iam')]:
        fixture = fixture.replace('https://' + host + '.googleapis.com/', server.host + '/' + route + '/')
        cloud_fixture = cloud_fixture.replace('https://' + host + '.googleapis.com/', server.host + '/' + route + '/')
    (test/'cloud.cpp').write_text(cloud_fixture)
    fixture = fixture.replace('CURLOPT_PROTOCOLS_STR,"https"', 'CURLOPT_PROTOCOLS_STR,"http,https"')
    fixture = fixture.replace('CURLOPT_VERBOSE,0L);', 'CURLOPT_VERBOSE,0L);\n        curl_easy_setopt(easy.get(),CURLOPT_PROXY,"");')
    fixture = fixture.replace('if (!m_impl->photo.starts_with("https://"))', 'if (!m_impl->photo.starts_with("http://127.0.0.1:"))')
    fixture = fixture.replace('L"LOCALAPPDATA"', 'L"STORAGE_OAUTH_TEST_VAULT"')
    (test / 'oauth.cpp').write_text(fixture)
    easy_fixture = (source / 'storage/google/easy/.cpp').read_text().replace('https://www.googleapis.com', server.host)
    # Only the fixture bypasses JWT signing for synthetic service credentials.
    # OAuth still follows the production token and DPoP paths above.
    easy_fixture = easy_fixture.replace('    auto token{', '''    if (!std::static_pointer_cast<google_account_t>(m_account_context)->appdata())
    {
        curl_easy_t::method();
        return true;
    }
    auto token{''', 1)
    (test / 'easy.cpp').write_text(easy_fixture)
    compile_source('storage/google/easy/.cpp', test / 'easy.cpp', test / 'easy.obj')
    compile_source('storage/google/oauth/.cpp', test / 'oauth.cpp', test / 'oauth.obj')
    compile_source('storage/google/cloud/.cpp', test / 'cloud.cpp', test / 'cloud.obj')
    compile_source('storage/storage.cpp', source / 'tests/oauth_login.cpp', test / 'test.obj')
    link(test / 'test.exe', test / 'test.obj', test / 'oauth.obj', test / 'easy.obj', cloud=test/'cloud.obj')
    link(test / 'production.exe', test / 'test.obj')
    for dll in (runtime / 'bin').glob('*.dll'):
        shutil.copy2(dll, test / dll.name)
    vault = test / 'vault'
    vault.mkdir()
    env = {**os.environ, 'WINEDEBUG': '-all', 'STORAGE_OAUTH_TEST_VAULT': 'Z:' + str(vault).replace('/', '\\')}

    def browser(cloud=False):
        path = test / 'browser-url'
        deadline = time.monotonic() + 20
        while not path.exists() and time.monotonic() < deadline:
            time.sleep(.02)
        assert path.exists(), 'Browser did not open.'
        query = urllib.parse.parse_qs(urllib.parse.urlsplit(path.read_text()).query)
        assert query['response_type'] == ['code'] and query['code_challenge_method'] == ['S256']
        assert query['prompt'] == ['consent'] and query['access_type'] == ['offline']
        assert query['scope'] == [cloud_scopes if cloud else 'https://www.googleapis.com/auth/drive.appdata']
        server.scope = query['scope'][0]
        assert len(unb64(query['state'][0])) == 32 and len(unb64(query['code_challenge'][0])) == 32
        server.challenge = query['code_challenge'][0]
        server.redirect = query['redirect_uri'][0]
        assert server.redirect.startswith('http://127.0.0.1:')
        def callback(state, extra=''):
            return urllib.request.urlopen(server.redirect + '?code=fixture-code&state=' + state + extra, timeout=5)
        for state, extra in [('wrong-state', ''), (query['state'][0], '&state=duplicate')]:
            try:
                callback(state, extra)
                raise AssertionError('Unsafe OAuth callback was accepted.')
            except urllib.error.HTTPError as error:
                assert error.code == 400
        with callback(query['state'][0]) as response:
            assert response.status == 200

    def run(mode, authorize=False, production=False):
        (test / 'browser-url').unlink(missing_ok=True)
        before = server.refreshes
        process = subprocess.Popen(['wine64', str(test / ('production.exe' if production else 'test.exe')), mode],
                                   cwd=test, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            if authorize:
                browser(cloud=mode in ('seed-cloud', 'cloud-denied'))
            stdout, stderr = process.communicate(timeout=30)
            assert not server.errors, server.errors
            assert process.returncode == 0, stdout + stderr
            assert 'fixture-access-' not in stdout + stderr and 'fixture-refresh-' not in stdout + stderr, 'Credentials leaked into diagnostics.'
            if mode in ('login', 'restore', 'relogin'):
                assert server.refreshes == before + (2 if mode == 'relogin' else 1), 'Parallel readers did not share exactly one refresh.'
            print(stdout.strip())
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()

    try:
        run('login', authorize=True)
        diagnostics = (test / 'oauth-http-log.txt').read_text()
        assert 'status=400' in diagnostics and 'Google OAuth HTTP' in diagnostics
        assert 'use_dpop_nonce' in diagnostics and '[REDACTED]' in diagnostics
        for secret in ('fixture-access-', 'fixture-refresh-', 'fixture-public-client-secret',
                       'fixture-code', 'DPoP: eyJ'):
            assert secret not in diagnostics, 'OAuth credentials leaked into HTTP diagnostics.'
        run('restore')
        run('relogin', authorize=True)
        # Three runs, each creating one blob and two reserve blobs (no appData clear).
        assert not server.deleted and server.created == 9
        server.email = 'second@example.invalid'
        run('second-login', authorize=True)
        server.email = 'oauth@example.invalid'
        run('seed-cloud', authorize=True)
        server.deny_scope = True
        run('cloud-denied', authorize=True)
        server.deny_scope = False
        run('cloud-project-interrupt')
        run('cloud-provision')
        # A separate process must reuse the ID, even with an empty search index.
        run('cloud-resume')
        for status in (403, 404):
            server.project_read_status = status
            run('cloud-lookup-error')
        server.project_read_status = 0
        run('cloud-resume')
        # A fresh workspace can still discover an existing project normally.
        shutil.rmtree(test/'cloud_project')
        server.search_visible = True
        run('cloud-resume')
        server.search_visible = False
        assert len(server.projects)==1 and server.revoked_keys==1
        assert server.cloud_rejections==1
        assert sum(method=='POST' and path=='/resource/v3/projects' for method,path in server.cloud_calls)==1
        assert sum(method=='POST' and '/fail-key%40' in path and path.endswith('/keys') for method,path in server.cloud_calls)==1
        files = [p for p in vault.rglob('*.json') if p.parent.name==hashlib.sha256(config['installed']['client_id'].encode()).hexdigest()]
        assert len(files) == 1
        envelope = files[0].read_text()
        assert 'fixture-access-' not in envelope and 'fixture-refresh-' not in envelope
        value = json.loads(envelope)
        cipher = unb64(value['cipher'])
        value['cipher'] = b64(bytes((cipher[0] ^ 1,)) + cipher[1:])
        files[0].write_text(json.dumps(value))
        run('reject')
        run('cancel')
        # The old service-account dropdown GUI was replaced by the space menus,
        # which run_spaces_gui.py exercises (Cloud provisioning, portraits,
        # sign-in/out and persistence across restarts).
        # Unmodified native implementation: synthetic config; Wine fails before
        # opening the browser or sending any network traffic. No secret fallback.
        native_config = json.loads(json.dumps(config))
        native_config['installed']['auth_uri'] = 'https://accounts.google.com/o/oauth2/auth'
        native_config['installed']['token_uri'] = 'https://oauth2.googleapis.com/token'
        (test / 'client.json').write_text(json.dumps(native_config))
        run('cng-failure', production=True)
        assert not server.errors
        print('PASS: PKCE, CSRF/duplicate callbacks, independently verified ES256 proofs, code-hash jti, nonce challenges, token rotation, encrypted vault and tamper rejection.')
    finally:
        server.shutdown()
        server.server_close()
