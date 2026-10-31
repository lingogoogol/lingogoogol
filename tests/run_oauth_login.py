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
        run('restore')
        run('relogin', authorize=True)
        assert server.deleted == {'appdata-a', 'appdata-b'} and server.created == 12
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
        (test / 'client.json').write_text(json.dumps(config))
        entry = (source / 'storage/storage.cpp').read_text(encoding='utf-8-sig')
        entry = entry.replace('import external.Vulkan;', 'import external.Vulkan;\nimport external.GLFW;')
        entry = entry.replace('progress_button_t','observed_t<progress_button_t>')
        entry = entry.replace('std::atomic_bool close_app', r'''template<class Base> struct observed_t : Base
        {
            using Base::Base;
            auto is_shown() const -> bool { return this->status()==this->shown; }
        };
        std::atomic_bool close_app''')
        entry = entry.replace('                auto target = interactive ?', r'''                if (!email.empty() && std::filesystem::exists("hold-account-switch"))
                {
                    storage_atomic_write("account-switch-started","ok");
                    while (std::filesystem::exists("hold-account-switch") && !settings_control->cancelled)
                        std::this_thread::sleep_for(std::chrono::milliseconds{5});
                    if (settings_control->cancelled) throw std::runtime_error{"Fixture switch cancelled."};
                }
                if (!email.empty() && std::filesystem::exists("fail-account-switch"))
                    throw std::runtime_error{"Fixture switch failed."};
                auto target = interactive ?''')

        observation = r'''        {
            // Under Wine, resizing the X11 wrapper does not reliably send
            // WM_SIZE. Use the production GLFW/Win32 resize path instead.
            if (std::filesystem::exists("resize-small"))
            {
                std::filesystem::remove("resize-small");
                glfwSetWindowSize(engine.window_get(),800,600);
            }
            if (std::filesystem::exists("resize-wide"))
            {
                std::filesystem::remove("resize-wide");
                glfwSetWindowSize(engine.window_get(),1536,1024);
            }
            if (std::filesystem::exists("resize-menu-sequence"))
            {
                std::filesystem::remove("resize-menu-sequence");
                // Win32's interactive sizing loop renders without returning to
                // the app's message_loop. Exercise that path between sizes.
                for (const auto size : {lgo::size_2D{1280,900},lgo::size_2D{960,720},
                    lgo::size_2D{800,600},lgo::size_2D{1100,800},lgo::size_2D{1536,1024}})
                {
                    glfwSetWindowSize(engine.window_get(),static_cast<int>(size.x),static_cast<int>(size.y));
                    if (engine.message_loop()) throw std::runtime_error{"GUI closed during resize."};
                    const auto panel = persistent.m_account_menu->get_object_shared<lgo::divy_soft_t>(1);
                    const auto anchor = persistent.m_login_button->get_pos();
                    const auto at = panel->get_pos();
                    const auto extent = panel->get_size();
                    if (engine.get_window_size()!=size || at.x+extent.x!=anchor.x+32 || at.y!=anchor.y+40 ||
                        at.y+extent.y>size.y-8)
                        throw std::runtime_error{"Dropdown did not follow the account button during resize."};
                }
                storage_atomic_write("resize-menu-sequence.done","ok");
            }
            if (std::filesystem::exists("lose-device"))
            {
                std::filesystem::remove("lose-device");
                engine.device_lost();
            }
            if (std::filesystem::exists("prepare-paused-task"))
            {
                std::filesystem::remove("prepare-paused-task");
                storage_atomic_write("paused-source.bin","resume this after logout");
                begin_task(&load_state,load_state.m_session,"upload-file","paused-source.bin",pos);
                load_state.m_task_list.back()->pause(true);
            }
            auto actions = nlohmann::json::array(), footer = nlohmann::json::array();
            auto account_rows = nlohmann::json::array();
            auto account_scroll = nlohmann::json{};
            std::function<void(lgo::GUI_object_t&)> inspect = [&](auto& object)
            {
                const auto p=object.get_pos(); const auto z=object.get_size();
                auto location=nlohmann::json{{"x",p.x+z.x/2},{"y",p.y+z.y/2}};
                if (auto button=dynamic_cast<progress_button_t*>(&object))
                {
                    location["text"]=button->get_text(); location["enabled"]=button->enabled();
                    actions.push_back(location);
                }
                if (auto button=dynamic_cast<lgo::button_classic_std_t*>(&object))
                {
                    location["enabled"]=button->enabled();
                    std::function<void(lgo::GUI_object_t&)> label = [&](auto& child)
                    {
                        if (auto text=dynamic_cast<lgo::text_line_t*>(&child)) location["text"]=text->get_text();
                        child.visit_children(label);
                    };
                    button->visit_children(label);
                    footer.push_back(location);
                }
                if (auto scroll=dynamic_cast<lgo::scroll_t*>(&object))
                {
                    const auto thumb=scroll->scrollbar_pos(false); const auto extent=scroll->scrollbar_size(false);
                    account_scroll={{"x",p.x},{"y",p.y},{"w",z.x},{"h",z.y},
                        {"view_h",scroll->view_size().y},{"content_h",scroll->content()->get_size().y},
                        {"max",scroll->max_scroll().y},{"offset",scroll->scroll_offset().y},
                        {"thumb_x",thumb.x},{"thumb_y",thumb.y},{"thumb_w",extent.x},{"thumb_h",extent.y}};
                }
                if (auto row=dynamic_cast<lgo::divx_soft_t*>(&object))
                {
                    auto item=nlohmann::json::object();
                    row->visit_children([&](lgo::GUI_object_t& child)
                    {
                        if (auto email=dynamic_cast<lgo::text_line_scroll_std_t*>(&child))
                            item["email"]=email->get_text();
                        if (auto rect=dynamic_cast<lgo::rect_t*>(&child);
                            rect && rect->get_size()==lgo::size_2D{32,32}) item["portrait"]=true;
                    });
                    if (item.contains("email")) account_rows.push_back(item);
                }
                object.visit_children(inspect);
            };
            if (persistent.m_account_menu)
                inspect(*persistent.m_account_menu->get_object_shared<lgo::divy_soft_t>(1));
            auto account_tasks = nlohmann::json::array();
            for (const auto& [id,dismissed] : settings_rows)
            {
                const auto row = progress_column->get_object_shared<lgo::divy_flex_soft_t>(id);
                auto task = nlohmann::json{{"text",row->get_object_shared<lgo::text_line_scroll_std_t>(0)->get_text()}};
                std::function<void(lgo::GUI_object_t&)> buttons = [&](auto& object)
                {
                    if (auto button=dynamic_cast<observed_t<progress_button_t>*>(&object))
                    {
                        const auto p=button->get_pos(); const auto z=button->get_size();
                        task[button->get_text()]={{"shown",button->is_shown()},{"x",p.x+z.x/2},{"y",p.y+z.y/2}};
                    }
                    object.visit_children(buttons);
                };
                buttons(*row);
                account_tasks.push_back(task);
            }
            auto signed_in_accounts = nlohmann::json::array();
            for (const auto& session : runtime.signed_in_sessions())
                signed_in_accounts.push_back({{"email",session->address},{"oauth",bool(session->oauth)}});
            const auto at=persistent.m_login_button->get_pos();
            const auto size=persistent.m_login_button->get_size();
            const auto window=engine.get_window_size();
            storage_atomic_write("oauth-ui.json",nlohmann::json{{"x",at.x},{"y",at.y},
                {"w",size.x},{"h",size.y},{"window_w",window.x},{"window_h",window.y},
                {"account_button_text",persistent.m_login_button->get_text()},
                {"signed_in",load_state.m_session && load_state.m_session->oauth},
                {"has_session",bool(load_state.m_session)},
                {"service_account",load_state.m_session && !load_state.m_session->oauth},
                {"account_path",load_state.m_session ? load_state.m_session->account_path : ""},
                {"root_id",load_state.m_session ? load_state.m_session->root : ""},
                {"account_rows",account_rows},{"signed_in_accounts",signed_in_accounts},
                {"switching",persistent.m_switching},{"menu",bool(persistent.m_account_menu)},{"cloud_menu",persistent.m_cloud_accounts},{"dialog",bool(persistent.m_dialog)},
                {"actions",actions},{"footer",footer},{"account_scroll",account_scroll},{"account_rows_settled",!progress_column->children_moving()},{"account_tasks",account_tasks},{"tasks",load_state.m_task_list.size()},
                {"paused",!load_state.m_task_list.empty() && load_state.m_task_list.front()->m_control->user_paused}}.dump());
        }
'''
        entry = entry.replace('        lgo::text_line_t::pending_load();', observation + '        lgo::text_line_t::pending_load();')
        (test / 'entry.cpp').write_text(entry)
        compile_source('storage/storage.cpp', test / 'entry.cpp', test / 'entry.obj')
        (test / 'bin').mkdir()
        (test / 'launch-cwd').mkdir()
        for dll in (runtime / 'bin').glob('*.dll'):
            shutil.copy2(dll, test / 'bin' / dll.name)
        link(test / 'bin/gui.exe', test / 'entry.obj', test / 'oauth.obj', test / 'easy.obj', gui=True, cloud=test/'cloud.obj')
        for resource in ('font', 'shader'):
            shutil.copytree(runtime / resource, test / resource)
        (test / 'root').mkdir()
        (test / 'blob_queue').mkdir()
        storage_id = 'appdata:' + hashlib.sha256(b'fixture.apps.googleusercontent.com').hexdigest() + ':oauth@example.invalid'
        (test / 'blob_queue' / hashlib.sha256(storage_id.encode()).hexdigest()).write_text(json.dumps(['oauth-gui-placeholder'] * 21000))
        root = test / 'root' / hashlib.sha256(storage_id.encode()).hexdigest()
        root.write_text('fixture-root')
        # The service-account root is preserved when Google login selects a user root.
        (test / 'root' / hashlib.sha256(b'mock@example.invalid').hexdigest()).write_text('original-service-account-root')
        (test / 'browser-url').unlink(missing_ok=True)
        interaction = r'''

import hashlib
import json,subprocess,sys,time
from pathlib import Path
from PIL import ImageGrab
app=subprocess.Popen(['wine64',sys.argv[1]],cwd='launch-cwd')
def command(*args):
    return subprocess.run(['xdotool',*map(str,args)],check=True,capture_output=True,text=True).stdout
def wait(predicate,message,timeout=25):
    deadline=time.monotonic()+timeout
    while time.monotonic()<deadline:
        try:
            value=predicate()
            if value:return value
        except (OSError,json.JSONDecodeError):pass
        assert app.poll() is None,'GUI exited early.'
        time.sleep(.05)
    raise AssertionError(message + ('; last UI: '+Path('oauth-ui.json').read_text() if Path('oauth-ui.json').exists() else ''))
def snapshot():return json.loads(Path('oauth-ui.json').read_text())
def click(at):command('mousemove','--window',window,at['x'],at['y'],'click','1')
def open_menu():
    at=snapshot();click({'x':at['x']+16,'y':at['y']+16})
    value=wait(lambda:snapshot() if snapshot()['menu'] else None,'Account dropdown missing.')
    assert value['footer'][0]['text']=='新增帳戶' and value['footer'][0]['enabled']
    assert all('portrait' not in row for row in value['account_rows']),'Account row retained a portrait.'
    return value
def close_menu():
    command('key','Escape');wait(lambda:not snapshot()['menu'],'Escape did not close the dropdown.')
def finished_account_task(name,prefix='已完成：'):
    value=snapshot()
    if not value['account_rows_settled']:return None
    return next((t for t in value['account_tasks'] if t['text'].startswith(prefix+name) and t['確定']['shown']),None)
def acknowledge(name,prefix='已完成：'):
    task=wait(lambda:finished_account_task(name,prefix),'Named account task did not finish: '+name)
    assert not task['取消']['shown'] and not task['重試']['shown']
    count=len(snapshot()['account_tasks'])
    click(task['確定'])
    wait(lambda:len(snapshot()['account_tasks'])==count-1 and snapshot()['account_rows_settled'],'Confirmation did not remove the account task.')
def restart():
    global app,window
    command('key','alt+F4');assert app.wait(timeout=10)==0
    Path('oauth-ui.json').unlink()
    app=subprocess.Popen(['wine64',sys.argv[1]],cwd='launch-cwd')
    wait(lambda:Path('oauth-ui.json').exists(),'Restart failed.')
    window=command('search','--onlyvisible','--name','wWinMain.engine').splitlines()[0]
    command('windowfocus','--sync',window)
try:
    wait(lambda:Path('oauth-ui.json').exists(),'Signed-out UI did not appear.')
    window=command('search','--onlyvisible','--name','wWinMain.engine').splitlines()[0]
    command('windowfocus','--sync',window)
    first=snapshot()
    assert not first['has_session'] and first['w']==first['h']==32 and first['account_button_text']=='登入'
    Path('resize-small').touch()
    narrow=wait(lambda:snapshot() if snapshot()['window_w']==800 else None,'Narrow layout did not appear.')
    assert narrow['x']+32<=800 and narrow['x']>=752
    menu=open_menu()
    assert not menu['actions'] and not menu['account_rows'] and len(menu['footer'])==1
    # Replace the service dropdown with the Cloud identity dropdown.
    click(menu['footer'][0])
    cloud=wait(lambda:snapshot() if snapshot()['menu'] and snapshot()['cloud_menu'] else None,'Cloud dropdown missing.')
    assert len(cloud['account_rows'])==1 and cloud['account_rows'][0]['portrait']
    assert [a['text'] for a in cloud['actions']]==['使用','移除'] and len(cloud['footer'])==1
    click(cloud['footer'][0])
    wait(lambda:Path('browser-url').exists(),'Cloud OAuth browser did not open.')
    import urllib.request,urllib.parse,os
    url=Path('browser-url').read_text()
    query=urllib.parse.parse_qs(urllib.parse.urlsplit(url).query)
    assert set(query['scope'][0].split())=={
        'https://www.googleapis.com/auth/cloud-platform',
        'https://www.googleapis.com/auth/userinfo.email','https://www.googleapis.com/auth/userinfo.profile'}
    host='http://127.0.0.1:'+os.environ['STORAGE_GPU_TEST_PORT']
    urllib.request.urlopen(host+'/fixture/authorize?url='+urllib.parse.quote(url,safe='')).close()
    urllib.request.urlopen(query['redirect_uri'][0]+'?code=fixture-code&state='+query['state'][0]).close()
    wait(lambda:snapshot()['menu'] and snapshot()['cloud_menu'] and not snapshot()['switching'],'Cloud login did not return to its dropdown.')
    assert not snapshot()['has_session'] and len(snapshot()['account_rows'])==1
    close_menu();acknowledge('新增 Google 使用者')
    restart()
    menu=open_menu();click(menu['footer'][0])
    cloud=wait(lambda:snapshot() if snapshot()['cloud_menu'] and snapshot()['menu'] else None,'Cloud history did not restore.')
    assert len(cloud['account_rows'])==1 and cloud['account_rows'][0]['portrait']
    ImageGrab.grab().save('/tmp/storage-cloud-accounts-menu.png')
    def use_cloud():
        click(snapshot()['actions'][0])
        wait(lambda:snapshot()['dialog'],'Service account name dialog missing.')
        current=snapshot()
        return (current['window_w']-720)//2,(current['window_h']-276)//2
    ox,oy=use_cloud()
    ImageGrab.grab().save('/tmp/storage-cloud-service-account-dialog.png')
    # Cancel after project/API preparation; no service account should be added.
    click({'x':ox+496,'y':oy+244})
    wait(lambda:not snapshot()['dialog'] and snapshot()['menu'],'Cancel did not return to Cloud users.')
    close_menu();acknowledge('準備服務帳戶專案：oauth@example.invalid')
    restart() # Cancelling the name dialog must retain the project across restart.
    menu=open_menu();click(menu['footer'][0]);wait(lambda:snapshot()['cloud_menu'] and snapshot()['menu'],'Cloud menu missing.')
    ox,oy=use_cloud()
    click({'x':ox+100,'y':oy+148});command('type','bad')
    click({'x':ox+616,'y':oy+244});time.sleep(.15)
    assert snapshot()['dialog'],'Invalid service account name was accepted.'
    click({'x':ox+100,'y':oy+148});command('key','ctrl+a');command('type','gui-service')
    click({'x':ox+616,'y':oy+244})
    wait(lambda:not snapshot()['dialog'] and not snapshot()['switching'] and snapshot()['menu'] and not snapshot()['cloud_menu'],
         'Service account creation did not return to the service dropdown.')
    assert not snapshot()['has_session'] and len(snapshot()['account_rows'])==1
    created=list(Path('key').glob('*-gui-service.json'));assert len(created)==1
    assert json.loads(created[0].read_text())['type']=='service_account'
    close_menu();acknowledge('準備服務帳戶專案：oauth@example.invalid');acknowledge('建立服務帳戶：gui-service')
    menu=open_menu();click(menu['footer'][0]);wait(lambda:snapshot()['cloud_menu'] and snapshot()['menu'],'Cloud menu missing before removal.')
    click(snapshot()['actions'][1])
    wait(lambda:snapshot()['menu'] and not snapshot()['switching'] and not snapshot()['account_rows'],'Cloud identity removal failed.')
    close_menu();acknowledge('移除 Google 使用者：oauth@example.invalid')
    created[0].unlink();Path('key').rmdir()
    Path('resize-wide').touch()
    wait(lambda:snapshot()['window_w']==1536,'Wide layout did not return.')
    Path('key').mkdir()
    services=[('arbitrary.data','aaa-service@example.invalid','service-root-A'),
              ('another file','oauth@example.invalid','service-root-B')]
    for filename,email,root_id in services:
        (Path('key')/filename).write_text(json.dumps({'client_email':email,'private_key':'fixture-key'}))
        digest=hashlib.sha256(email.encode()).hexdigest()
        (Path('root')/digest).write_text(root_id)
        (Path('blob_queue')/digest).write_text(json.dumps(['service-placeholder']*21000))
    Path('key/broken.json').write_text('{')
    original_keys={p.name:p.read_bytes() for p in Path('key').iterdir()}
    menu=open_menu()
    assert [r['email'] for r in menu['account_rows']]==[s[1] for s in services]
    assert [a['text'] for a in menu['actions']]==['登入','登出']*2
    assert not any(menu['actions'][i]['enabled'] for i in (1,3))
    bar=menu['account_scroll']
    assert bar['max']==0 and bar['view_h']==bar['content_h'],'Short account list has an unnecessary scrollbar.'
    close_menu()
    def check_account_bar(menu):
        bar=menu['account_scroll']
        assert bar['view_h']==min(bar['content_h'],max(0,menu['window_h']-menu['y']-112)), bar
        assert bar['max']==max(0,bar['content_h']-bar['view_h']), bar
        length=bar['view_h']
        expected=max(min(24,length),min(length,length*length//bar['content_h']))
        assert bar['thumb_h']==expected and bar['thumb_w']==2, bar
        assert bar['thumb_x']==bar['x']+bar['w']-2, bar
        assert bar['thumb_y']==bar['y']+bar['offset']*(length-expected)//bar['max'], bar
        return bar
    extras=[]
    for i in range(30):
        path=Path('key')/('scroll-'+str(i)+'.json')
        path.write_text(json.dumps({'client_email':'scroll-'+str(i)+'@example.invalid','private_key':'fixture-key'}))
        extras.append(path)
    menu=open_menu();assert len(menu['actions'])==64
    bar=check_account_bar(menu)
    last_y=menu['actions'][-1]['y']
    click_at={'x':menu['actions'][0]['x']-150,'y':menu['actions'][0]['y']}
    command('mousemove','--window',window,click_at['x'],click_at['y'],'click','--repeat','8','--delay','50','5')
    wait(lambda:snapshot()['actions'][-1]['y']<last_y-100,'Account list did not scroll.')
    bar=check_account_bar(snapshot())
    # The track and thumb must follow pointer input as well as wheel scrolling.
    click({'x':bar['thumb_x']+1,'y':bar['y']+bar['view_h']-1})
    wait(lambda:snapshot()['account_scroll']['offset']==snapshot()['account_scroll']['max'],
         'Account scrollbar track did not reach the bottom.')
    bar=check_account_bar(snapshot())
    command('mousemove','--window',window,bar['thumb_x']+1,bar['thumb_y']+bar['thumb_h']//2,'mousedown','1')
    command('mousemove','--sync','--window',window,bar['thumb_x']+1,bar['y'],'mouseup','1')
    wait(lambda:snapshot()['account_scroll']['offset']==0,'Account scrollbar drag did not reach the top.')
    bar=check_account_bar(snapshot())
    command('mousemove',1500,1100)
    def scrollbar_pixels():
        current=snapshot()['account_scroll']
        geometry=dict(line.split('=',1) for line in command('getwindowgeometry','--shell',window).splitlines())
        image=ImageGrab.grab()
        for y,rgb in ((current['thumb_y']+current['thumb_h']//2,(0,0,0)),
                      (current['y']+current['view_h']-1,(211,211,211))):
            pixel=image.getpixel((int(geometry['X'])+current['thumb_x']+1,int(geometry['Y'])+y))
            if any(abs(a-b)>8 for a,b in zip(pixel[:3],rgb)):return None
        return image
    wait(scrollbar_pixels,'Account scrollbar pixels did not match its geometry.').save('/tmp/storage-account-scrollbar.png')
    Path('resize-menu-sequence').touch()
    wait(lambda:Path('resize-menu-sequence.done').exists(),'Dropdown did not follow continuous resizing.')
    Path('resize-small').touch()
    wait(lambda:snapshot()['window_w']==800 and all(0<a['x']<800 and 0<a['y']<600
         for a in snapshot()['footer']),'Open dropdown did not resize within the window.')
    small=check_account_bar(snapshot())
    Path('resize-wide').touch()
    wide=wait(lambda:snapshot() if snapshot()['window_w']==1536 else None,'Open dropdown did not expand with the window.')
    expanded=check_account_bar(wide)
    assert expanded['view_h']>small['view_h'] and expanded['thumb_h']>small['thumb_h']
    command('key','Escape');wait(lambda:not snapshot()['menu'],'Large dropdown did not close.')
    # Opening at a small size must not permanently cap the list after expansion.
    Path('resize-small').touch()
    wait(lambda:snapshot()['window_w']==800,'Small layout did not return.')
    small=check_account_bar(open_menu())
    Path('resize-wide').touch()
    wide=wait(lambda:snapshot() if snapshot()['window_w']==1536 else None,'Initially small dropdown did not expand.')
    assert check_account_bar(wide)['view_h']>small['view_h']
    command('key','Escape');wait(lambda:not snapshot()['menu'],'Expanded dropdown did not close.')

    for path in extras:path.unlink()
    # Duplicate emails remain separate rows and sort by credential path.
    duplicate=Path('key/duplicate.json');duplicate.write_bytes(Path('key/arbitrary.data').read_bytes())
    menu=open_menu()
    assert [r['email'] for r in menu['account_rows']]==[services[0][1],services[0][1],services[1][1]]
    assert len(menu['actions'])==6
    close_menu();duplicate.unlink()
    for i,(filename,email,root_id) in enumerate(services):
        menu=open_menu();click(menu['actions'][2*i])
        wait(lambda:snapshot()['service_account'] and not snapshot()['switching'] and
                    snapshot()['account_path']=='key/'+filename and snapshot()['root_id']==root_id,'Wrong service account selected.')
        assert snapshot()['account_button_text']=='帳戶' and snapshot()['tasks']==0
        acknowledge('切換帳戶：'+email)
    assert len(snapshot()['signed_in_accounts'])==2
    restart()
    assert len(snapshot()['signed_in_accounts'])==2 and snapshot()['root_id']=='service-root-B' and snapshot()['tasks']==0
    switch_name='切換帳戶：'+services[0][1]
    menu=open_menu();click(menu['actions'][0])
    wait(lambda:finished_account_task(switch_name),'Completed switch did not retain its task.')
    Path('hold-account-switch').touch()
    menu=open_menu();click(menu['actions'][0])
    wait(lambda:Path('account-switch-started').exists(),'Held switch did not start.')
    acknowledge(switch_name)
    assert snapshot()['switching'],'Acknowledging an old result stopped the current switch.'
    click(snapshot()['account_tasks'][-1]['取消'])
    acknowledge(switch_name,'已取消：');Path('hold-account-switch').unlink()
    Path('fail-account-switch').touch()
    menu=open_menu();click(menu['actions'][0]);acknowledge(switch_name,'失敗：')
    Path('fail-account-switch').unlink()
    for i,(filename,email,root_id) in enumerate(services):
        menu=open_menu();click(menu['actions'][2*i])
        wait(lambda:not snapshot()['switching'] and snapshot()['root_id']==root_id,'Service reselection failed.')
        acknowledge('切換帳戶：'+email)
        Path('prepare-paused-task').touch()
        wait(lambda:snapshot()['tasks']==i+1 and snapshot()['paused'],'Paused service task was not created.')
    index=Path('task').read_text()
    assert len(json.loads(index))==2
    menu=open_menu();click(menu['actions'][1])
    wait(lambda:not snapshot()['switching'] and snapshot()['tasks']==1,'Background service logout affected another account.')
    assert snapshot()['root_id']=='service-root-B'
    acknowledge('登出：'+services[0][1])
    menu=open_menu();assert not menu['actions'][1]['enabled'] and menu['actions'][3]['enabled']
    click(menu['actions'][1]);time.sleep(.2)
    assert snapshot()['menu'] and not snapshot()['switching'],'Disabled logout handled a click.'
    close_menu()
    Path('lose-device').touch();wait(lambda:not Path('lose-device').exists(),'GPU recovery was not requested.')
    wait(lambda:snapshot()['account_button_text']=='帳戶','Account text did not survive GPU recovery.')
    menu=open_menu()
    command('mousemove',1500,1100)
    ImageGrab.grab().save('/tmp/storage-service-accounts-menu.png')
    click(menu['actions'][3])
    wait(lambda:not snapshot()['has_session'] and not snapshot()['switching'] and snapshot()['tasks']==0,'Current service logout failed.')
    acknowledge('登出：'+services[1][1])
    assert Path('task').read_text()==index and snapshot()['account_button_text']=='登入'
    restart()
    assert not snapshot()['has_session'] and snapshot()['tasks']==0
    menu=open_menu();assert len(menu['footer'])==1 and not any(menu['actions'][i]['enabled'] for i in (1,3))
    click(menu['actions'][2])
    wait(lambda:snapshot()['service_account'] and not snapshot()['switching'] and snapshot()['tasks']==1,'Explicit relogin did not restore its task.')
    assert snapshot()['paused'];acknowledge('切換帳戶：'+services[1][1])
    restart()
    assert snapshot()['service_account'] and snapshot()['tasks']==1 and snapshot()['paused']
    menu=open_menu();click(menu['actions'][0])
    wait(lambda:not snapshot()['switching'] and snapshot()['tasks']==2,'Second service relogin failed.')
    acknowledge('切換帳戶：'+services[0][1])
    menu=open_menu();click(menu['actions'][1])
    wait(lambda:not snapshot()['has_session'] and not snapshot()['switching'] and snapshot()['tasks']==1,'Selected logout stopped a background account.')
    acknowledge('登出：'+services[0][1])
    restart()
    assert not snapshot()['has_session'] and len(snapshot()['signed_in_accounts'])==1 and snapshot()['tasks']==1
    menu=open_menu();assert len(menu['footer'])==2 and menu['actions'][3]['enabled']
    click(menu['footer'][1])
    wait(lambda:not snapshot()['switching'] and not snapshot()['signed_in_accounts'] and snapshot()['tasks']==0,'Sign out all failed.')
    acknowledge('登出所有');assert Path('task').read_text()==index
    restart();assert not snapshot()['has_session'] and snapshot()['tasks']==0
    assert original_keys=={p.name:p.read_bytes() for p in Path('key').iterdir()},'Credentials were modified.'
    for path in Path('key').iterdir():path.unlink()
    Path('key').rmdir()
    menu=open_menu();assert not menu['account_rows'] and not menu['actions'] and len(menu['footer'])==1
    close_menu()
    command('key','alt+F4');assert app.wait(timeout=10)==0
    assert not list(Path('launch-cwd').iterdir()),'GUI wrote state into the launching CWD.'
    assert all(p.suffix in ('.exe','.dll') for p in Path('bin').iterdir()),'GUI wrote resources or state into bin.'
    assert all(len(p.name)==64 and all(c in '0123456789abcdef' for c in p.name)
               for folder in ('root','blob_queue') for p in Path(folder).iterdir())
    print('PASS: service-only dropdown, text account entry, no portraits, Cloud provisioning submenu, portraits and account creation; service discovery/sorting/duplicate emails, scrolling/resizing, GPU recovery, named switch results, isolated logout, paused checkpoints and login persistence across restarts.')
finally:
    if app.poll() is None:app.kill();app.wait()
'''
        process = subprocess.Popen(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24',
                                    'python3', '-c', interaction, str(test / 'bin/gui.exe')],
                                   cwd=test, env={**env, 'STORAGE_GPU_TEST_PORT': str(server.server_port)},
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        photos_before_gui = getattr(server, 'photos', 0)
        try:
            stdout, stderr = process.communicate(timeout=120)
            assert process.returncode == 0, stdout + stderr
            assert not server.errors, server.errors
            logs = ''.join(p.read_text(errors='replace') for p in (test / 'logfile').glob('*.txt'))
            assert 'finished_all!' in logs and 'uncaught exception:' not in logs
            assert 'fixture-access-' not in logs + stdout + stderr
            assert 'fixture-refresh-' not in logs + stdout + stderr
            assert 'fixture-cloud-private-key' not in logs + stdout + stderr
            assert getattr(server, 'photos', 0) == photos_before_gui+1, 'Cloud login must download exactly one portrait.'
            assert (test / 'browser-url').exists(), 'Cloud login did not launch OAuth.'
            assert len(server.projects)==1, 'Cancelling/restarting created a second project while search lagged.'
            assert sum(method=='POST' and path=='/resource/v3/projects' for method,path in server.cloud_calls)==1
            print(stdout.strip())
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
        # Two OAuth users without tasks exercise restoration by each saved email,
        # rather than the vault's single legacy "active" pointer.
        multi_accounts = {}
        for i,email in enumerate(('first-multi@example.invalid','second-multi@example.invalid')):
            server.email = email
            run('seed-login',authorize=True)
            identity = 'appdata:' + hashlib.sha256(b'fixture.apps.googleusercontent.com').hexdigest() + ':' + email
            multi_accounts[identity] = {'address':email,'account_path':'client.json','root_directory':'root','oauth':True}
            digest = hashlib.sha256(identity.encode()).hexdigest()
            (test/'root'/digest).write_text('multi-root-'+str(i))
            (test/'blob_queue'/digest).write_text(json.dumps(['multi-placeholder']*21000))
        server.email = 'oauth@example.invalid'
        multi_settings = json.loads((test/'settings.json').read_text())
        multi_settings['account_path'] = 'client.json'
        multi_settings['signed_in_accounts'] = multi_accounts
        multi_settings['selected_account'] = next(iter(multi_accounts))
        (test/'settings.json').write_text(json.dumps(multi_settings))
        (test/'oauth-ui.json').unlink()
        multi_interaction = r'''
import json,subprocess,sys,time
from pathlib import Path
app=None
window=None
def command(*args):return subprocess.check_output(['xdotool',*map(str,args)],text=True)
def snapshot():return json.loads(Path('oauth-ui.json').read_text())
def wait(predicate,message):
    deadline=time.monotonic()+25
    while time.monotonic()<deadline:
        try:
            value=predicate()
            if value:return value
        except (OSError,json.JSONDecodeError):pass
        assert app.poll() is None,'Multi-user GUI exited early.'
        time.sleep(.05)
    raise AssertionError(message+'; last UI: '+Path('oauth-ui.json').read_text())
def launch():
    global app,window
    app=subprocess.Popen(['wine64',sys.argv[1]],cwd='launch-cwd')
    wait(lambda:Path('oauth-ui.json').exists(),'Multi-user startup failed.')
    window=command('search','--onlyvisible','--name','wWinMain.engine').splitlines()[0]
    command('windowfocus','--sync',window)
def click(at):command('mousemove','--window',window,at['x'],at['y'],'click','1')
def menu():
    at=snapshot();click({'x':at['x']+16,'y':at['y']+16})
    return wait(lambda:snapshot() if snapshot()['menu'] else None,'Multi-user menu did not open.')
def restart():
    command('key','alt+F4');assert app.wait(timeout=10)==0
    Path('oauth-ui.json').unlink();launch()
try:
    launch()
    assert len(snapshot()['signed_in_accounts'])==2 and snapshot()['tasks']==0 and snapshot()['root_id']=='multi-root-0'
    current=menu()
    assert not current['account_rows'] and not current['actions']
    assert len(current['footer'])==2 and current['footer'][0]['enabled']
    click(current['footer'][0]);wait(lambda:snapshot()['menu'] and snapshot()['cloud_menu'],'Cloud submenu missing with background OAuth sessions.')
    assert not snapshot()['account_rows'] and len(snapshot()['footer'])==1 and len(snapshot()['signed_in_accounts'])==2
    command('key','Escape');wait(lambda:not snapshot()['menu'],'Multi-user menu did not close.')
    restart()
    assert len(snapshot()['signed_in_accounts'])==2 and snapshot()['root_id']=='multi-root-0' and snapshot()['tasks']==0
    current=menu()
    assert not current['account_rows'] and not current['actions'] and len(current['footer'])==2
    click(current['footer'][1])
    wait(lambda:not snapshot()['switching'] and not snapshot()['signed_in_accounts'],'Multi-user sign out all failed.')
    restart()
    assert not snapshot()['has_session'] and not snapshot()['signed_in_accounts'] and snapshot()['tasks']==0
    current=menu()
    assert not current['account_rows'] and not current['actions'] and len(current['footer'])==1
    command('key','alt+F4');assert app.wait(timeout=10)==0
    print('PASS: existing OAuth logins restore but remain hidden from the service-only dropdown; Cloud submenu and sign out all survive restart.')
finally:
    if app is not None and app.poll() is None:app.kill();app.wait()
'''
        photos_before_gui = getattr(server, 'photos', 0)
        result = subprocess.run(['xvfb-run','-a','-s','-screen 0 1600x1200x24','python3','-c',multi_interaction,str(test/'bin/gui.exe')],
                                cwd=test,env=env,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True,timeout=100)
        assert result.returncode==0,result.stdout+result.stderr
        assert not server.errors,server.errors
        assert getattr(server, 'photos', 0) == photos_before_gui, 'Restored OAuth account downloaded a profile photo.'
        print(result.stdout.strip())
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
