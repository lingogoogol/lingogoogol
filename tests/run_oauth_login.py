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
                                    'scope': 'https://www.googleapis.com/auth/drive.appdata'}, 'success-nonce')
        except Exception:
            self.server.errors.append(traceback.format_exc())
            self.reply(500, {'error': 'fixture-validation'})


def compile_source(original, replacement, output):
    entries = json.loads((build / 'compile_commands.json').read_text())
    entry = next(item for item in entries if Path(item['file']) == source / original)
    command = shlex.split(entry['command'])
    command[command.index('-o') + 1] = str(output)
    command[command.index('-c') + 1] = str(replacement)
    subprocess.run(command, cwd=build, check=True)


def link(output, entry, oauth=None, easy=None, gui=False):
    text = subprocess.check_output(['ninja', '-t', 'commands', 'storage.exe'], cwd=build, text=True).splitlines()[-1]
    command = shlex.split(text.split(' && ')[1])
    if not gui:
        command = [arg for arg in command if arg not in ('-mwindows', '-municode') and not arg.startswith('-Wl,--out-implib')]
    command[command.index('-o') + 1] = str(output)
    command[command.index('storage/CMakeFiles/storage.dir/storage.cpp.obj')] = str(entry)
    for target, obj in [('oauth', oauth), ('easy', easy)]:
        if obj:
            command[command.index('storage/google/' + target + '/libstorage.google.' + target + '.a')] = str(obj)
            command.append('storage/google/' + target + '/CMakeFiles/storage.google.' + target + '.dir/.cppm.obj')
    subprocess.run(command, cwd=build, check=True)


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
    fixture = fixture.replace('CURLOPT_PROTOCOLS_STR,"https"', 'CURLOPT_PROTOCOLS_STR,"http,https"')
    fixture = fixture.replace('CURLOPT_VERBOSE,0L);', 'CURLOPT_VERBOSE,0L);\n        curl_easy_setopt(easy.get(),CURLOPT_PROXY,"");')
    fixture = fixture.replace('if (!m_impl->photo.starts_with("https://"))', 'if (!m_impl->photo.starts_with("http://127.0.0.1:"))')
    fixture = fixture.replace('L"LOCALAPPDATA"', 'L"STORAGE_OAUTH_TEST_VAULT"')
    (test / 'oauth.cpp').write_text(fixture)
    easy_fixture = (source / 'storage/google/easy/.cpp').read_text().replace('https://www.googleapis.com', server.host)
    (test / 'easy.cpp').write_text(easy_fixture)
    compile_source('storage/google/easy/.cpp', test / 'easy.cpp', test / 'easy.obj')
    compile_source('storage/google/oauth/.cpp', test / 'oauth.cpp', test / 'oauth.obj')
    compile_source('storage/storage.cpp', source / 'tests/oauth_login.cpp', test / 'test.obj')
    link(test / 'test.exe', test / 'test.obj', test / 'oauth.obj', test / 'easy.obj')
    link(test / 'production.exe', test / 'test.obj')
    for dll in (runtime / 'bin').glob('*.dll'):
        shutil.copy2(dll, test / dll.name)
    vault = test / 'vault'
    vault.mkdir()
    env = {**os.environ, 'WINEDEBUG': '-all', 'STORAGE_OAUTH_TEST_VAULT': 'Z:' + str(vault).replace('/', '\\')}

    def browser():
        path = test / 'browser-url'
        deadline = time.monotonic() + 20
        while not path.exists() and time.monotonic() < deadline:
            time.sleep(.02)
        assert path.exists(), 'Browser did not open.'
        query = urllib.parse.parse_qs(urllib.parse.urlsplit(path.read_text()).query)
        assert query['response_type'] == ['code'] and query['code_challenge_method'] == ['S256']
        assert query['prompt'] == ['consent'] and query['access_type'] == ['offline']
        assert query['scope'] == ['https://www.googleapis.com/auth/drive.appdata']
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
                browser()
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
        files = list(vault.rglob('*.json'))
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
                        throw std::runtime_error{"Dropdown did not follow the profile during resize."};
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
                if (dynamic_cast<lgo::button_classic_std_t*>(&object)) footer.push_back(location);
                if (auto scroll=dynamic_cast<lgo::scroll_t*>(&object))
                {
                    const auto thumb=scroll->scrollbar_pos(false); const auto extent=scroll->scrollbar_size(false);
                    account_scroll={{"x",p.x},{"y",p.y},{"w",z.x},{"h",z.y},
                        {"view_h",scroll->view_size().y},{"content_h",scroll->content()->get_size().y},
                        {"max",scroll->max_scroll().y},{"offset",scroll->scroll_offset().y},
                        {"thumb_x",thumb.x},{"thumb_y",thumb.y},{"thumb_w",extent.x},{"thumb_h",extent.y}};
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
            const auto at=persistent.m_login_button->get_pos();
            const auto size=persistent.m_login_button->get_size();
            const auto window=engine.get_window_size();
            storage_atomic_write("oauth-ui.json",nlohmann::json{{"x",at.x},{"y",at.y},
                {"w",size.x},{"h",size.y},{"window_w",window.x},{"window_h",window.y},
                {"signed_in",load_state.m_session && load_state.m_session->oauth},
                {"switching",persistent.m_switching},{"menu",bool(persistent.m_account_menu)},
                {"actions",actions},{"footer",footer},{"account_scroll",account_scroll},{"account_rows_settled",!progress_column->children_moving()},{"account_tasks",account_tasks},{"tasks",load_state.m_task_list.size()},
                {"paused",!load_state.m_task_list.empty() && load_state.m_task_list.front()->m_control->user_paused}}.dump());
        }
'''
        entry = entry.replace('return google_oauth_user_t::saved_accounts(candidate.account_path);', r'''auto accounts = google_oauth_user_t::saved_accounts(candidate.account_path);
        if (std::filesystem::exists("few-accounts") && !accounts.empty())
            accounts.push_back({"few@example.invalid",accounts.front().portrait});
        if (std::filesystem::exists("many-accounts") && !accounts.empty())
            for (int i{};i<30;++i) accounts.push_back({"fixture-"+std::to_string(i)+"@example.invalid",accounts.front().portrait});
        return accounts;''')
        entry = entry.replace('        lgo::text_line_t::pending_load();', observation + '        lgo::text_line_t::pending_load();')
        (test / 'entry.cpp').write_text(entry)
        compile_source('storage/storage.cpp', test / 'entry.cpp', test / 'entry.obj')
        (test / 'bin').mkdir()
        (test / 'launch-cwd').mkdir()
        for dll in (runtime / 'bin').glob('*.dll'):
            shutil.copy2(dll, test / 'bin' / dll.name)
        link(test / 'bin/gui.exe', test / 'entry.obj', test / 'oauth.obj', test / 'easy.obj', gui=True)
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
try:
    wait(lambda:Path('oauth-ui.json').exists(),'Signed-out UI did not appear.')
    window=command('search','--onlyvisible','--name','wWinMain.engine').splitlines()[0]
    command('windowfocus','--sync',window)
    first=snapshot()
    assert not first['signed_in'] and first['w']==first['h']==32
    assert first['x']+32<=first['window_w'] and first['x']>=first['window_w']-48
    Path('resize-small').touch()
    narrow=wait(lambda:snapshot() if snapshot()['window_w']==800 else None,'Narrow layout did not appear.')
    assert narrow['x']+32<=800 and narrow['x']>=752
    command('mousemove','--window',window,narrow['x']+16,narrow['y']+16,'click','1')
    menu=wait(lambda:snapshot() if snapshot()['menu'] else None,'Signed-out dropdown missing.')
    assert not menu['actions'] and len(menu['footer'])==1
    def click(at):command('mousemove','--window',window,at['x'],at['y'],'click','1')
    click(menu['footer'][0])
    wait(lambda:snapshot()['signed_in'] and not snapshot()['switching'],'Google login did not become active.')
    command('mousemove',1500,1100) # Remove the cursor highlight before checking photo colors.
    def colored():
        at=snapshot()
        geometry=dict(line.split('=',1) for line in command('getwindowgeometry','--shell',window).splitlines())
        image=ImageGrab.grab()
        left=image.getpixel((int(geometry['X'])+at['x']+8,int(geometry['Y'])+at['y']+16))
        right=image.getpixel((int(geometry['X'])+at['x']+24,int(geometry['Y'])+at['y']+16))
        correct=all(abs(a-b)<8 for a,b in zip(left[:3],(240,40,30))) and all(abs(a-b)<8 for a,b in zip(right[:3],(30,210,70)))
        return image if correct else None
    image=wait(colored,'Profile photo was not rendered in color.')
    image.save('/tmp/storage-oauth-profile.png')
    Path('lose-device').touch()
    wait(lambda:not Path('lose-device').exists(),'GPU recovery was not requested.')
    time.sleep(1)
    wait(colored,'Profile photo did not survive GPU recovery.')
    Path('resize-wide').touch()
    wide=wait(lambda:snapshot() if snapshot()['window_w']==1536 else None,'Wide layout did not return.')
    assert wide['w']==wide['h']==32 and wide['x']+32<=1536
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
    def open_menu():
        at=snapshot();click({'x':at['x']+16,'y':at['y']+16})
        return wait(lambda:snapshot() if snapshot()['menu'] else None,'Account dropdown missing.')
    menu=open_menu()
    assert [a['text'] for a in menu['actions']]==['登入','移除']
    assert menu['actions'][0]['enabled'] and not menu['actions'][1]['enabled']
    click(menu['actions'][1]);time.sleep(.2)
    assert snapshot()['menu'] and snapshot()['signed_in'],'Disabled removal handled a click.'
    click(menu['actions'][0])
    switch_name='切換帳戶：oauth@example.invalid'
    wait(lambda:finished_account_task(switch_name),'Switch did not retain its task name.')
    acknowledge('新增帳戶：oauth@example.invalid')
    Path('hold-account-switch').touch()
    menu=open_menu();click(menu['actions'][0])
    wait(lambda:Path('account-switch-started').exists(),'Held switch did not start.')
    running=snapshot()['account_tasks'][-1]
    assert running['text']==switch_name+'（處理中）' and running['取消']['shown'] and not running['確定']['shown']
    acknowledge(switch_name)  # Acknowledging the previous row must not cancel this switch.
    assert snapshot()['switching']
    running=snapshot()['account_tasks'][-1];click(running['取消'])
    acknowledge(switch_name,'已取消：')
    assert snapshot()['signed_in'] and not snapshot()['switching']
    Path('hold-account-switch').unlink()
    Path('fail-account-switch').touch()
    menu=open_menu();click(menu['actions'][0])
    acknowledge(switch_name,'失敗：')
    Path('fail-account-switch').unlink()
    menu=open_menu()
    command('mousemove',1500,1100)
    ImageGrab.grab().save('/tmp/storage-account-dropdown.png')
    command('key','Escape')
    wait(lambda:not snapshot()['menu'],'Escape did not dismiss dropdown.')
    menu=open_menu();click({'x':700,'y':400})
    wait(lambda:not snapshot()['menu'],'Outside click did not dismiss dropdown.')
    Path('few-accounts').touch()
    menu=open_menu();bar=menu['account_scroll']
    assert bar['max']==0 and bar['view_h']==bar['content_h'], 'Short account list has an unnecessary scrollbar: '+str(bar)
    command('key','Escape');wait(lambda:not snapshot()['menu'],'Short dropdown did not close.')
    Path('few-accounts').unlink()
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
    Path('many-accounts').touch()
    menu=open_menu();assert len(menu['actions'])==62
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
    Path('many-accounts').unlink()
    Path('resize-wide').touch()
    wait(lambda:snapshot()['window_w']==1536,'Wide layout did not return after dropdown resize.')
    Path('prepare-paused-task').touch()
    wait(lambda:snapshot()['tasks']==1 and snapshot()['paused'],'Paused fixture was not created.')
    index=Path('task').read_text()
    menu=open_menu();assert len(menu['footer'])==2
    click(menu['footer'][1])
    wait(lambda:not snapshot()['signed_in'] and not snapshot()['switching'],'Logout failed.')
    acknowledge('登出：oauth@example.invalid')
    assert Path('task').read_text()==index,'Logout deleted the transfer checkpoint.'
    command('key','alt+F4');app.wait(timeout=10)
    assert app.returncode==0
    checkpoint=Path('task.data')/json.loads(index)[0]/'state'
    saved=json.loads(checkpoint.read_text())
    assert saved.pop('root_directory')=='root','New checkpoint did not save the root directory.'
    saved['root_path']='root.appdata.'+'a'*64  # Older journals saved a root file path.
    checkpoint.write_text(json.dumps(saved))
    Path('oauth-ui.json').unlink()
    app=subprocess.Popen(['wine64',sys.argv[1]],cwd='launch-cwd')
    wait(lambda:Path('oauth-ui.json').exists(),'Signed-out restart failed.')
    assert not snapshot()['signed_in'] and snapshot()['tasks']==0,'Restart resumed a signed-out account.'
    window=command('search','--onlyvisible','--name','wWinMain.engine').splitlines()[0]
    command('windowfocus','--sync',window)
    menu=open_menu();assert menu['actions'][1]['enabled'] and len(menu['footer'])==1
    click(menu['actions'][0])
    wait(lambda:snapshot()['signed_in'] and not snapshot()['switching'] and snapshot()['tasks']==1,
         'Saved-account login did not restore the transfer.')
    assert snapshot()['paused'],'Login lost explicit pause state.'
    menu=open_menu();click(menu['footer'][1])
    wait(lambda:not snapshot()['signed_in'] and not snapshot()['switching'],'Second logout failed.')
    acknowledge(switch_name)
    acknowledge('登出：oauth@example.invalid')
    menu=open_menu();click(menu['actions'][1])
    wait(lambda:not snapshot()['switching'] and not snapshot()['menu'],'Removal did not finish.')
    acknowledge('移除帳戶：oauth@example.invalid')
    menu=open_menu();assert not menu['actions'] and len(menu['footer'])==1,'Removed account remains in history.'
    assert Path('task').read_text()==index,'Removal deleted the transfer checkpoint.'
    command('key','alt+F4')
    app.wait(timeout=10)
    assert app.returncode==0
    assert (Path('root') / hashlib.sha256(b'mock@example.invalid').hexdigest()).read_text()=='original-service-account-root'
    assert not list(Path('launch-cwd').iterdir()),'GUI wrote state into the launching CWD.'
    assert all(p.suffix in ('.exe','.dll') for p in Path('bin').iterdir()),'GUI wrote resources or state into bin.'
    assert all(len(p.name)==64 and all(c in '0123456789abcdef' for c in p.name)
               for folder in ('root','blob_queue') for p in Path(folder).iterdir()),'Non-hash state filenames were created.'
    print('PASS: account scrollbar extent, thumb pixels, wheel/track/drag and resize geometry; named account tasks, success/cancel/failure acknowledgement and independent old rows; bin executable uses its parent from an unrelated CWD; signed-in/out account dropdown, disabled active removal, cached login, durable logout, checkpoint restoration, removal, photo color/GPU recovery and clean close.')
finally:
    if app.poll() is None:app.kill();app.wait()
'''
        process = subprocess.Popen(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24',
                                    'python3', '-c', interaction, str(test / 'bin/gui.exe')],
                                   cwd=test, env={**env, 'STORAGE_GPU_TEST_PORT': str(server.server_port)},
                                   stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        try:
            try:
                browser()
            except Exception:
                try:
                    stdout, stderr = process.communicate(timeout=10)
                except subprocess.TimeoutExpired:
                    process.kill()
                    stdout, stderr = process.communicate()
                raise AssertionError('GUI did not launch browser: ' + stdout + stderr)
            stdout, stderr = process.communicate(timeout=60)
            assert process.returncode == 0, stdout + stderr
            assert not server.errors, server.errors
            logs = ''.join(p.read_text(errors='replace') for p in (test / 'logfile').glob('*.txt'))
            assert 'finished_all!' in logs and 'uncaught exception:' not in logs
            assert 'fixture-access-' not in logs + stdout + stderr
            assert 'fixture-refresh-' not in logs + stdout + stderr
            print(stdout.strip())
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()
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
