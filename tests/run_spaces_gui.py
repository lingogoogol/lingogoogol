#!/usr/bin/env python3
"""Exercise the actual space menus and Cloud provisioning using local fixtures."""
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import subprocess
import tempfile
import threading
import time
import urllib.parse
import urllib.request

source = Path(__file__).resolve().parents[1]
# Reuse the OAuth protocol/KSP fixture without executing its legacy account UI
# scenarios. That fixture verifies PKCE, DPoP, nonce and token refresh itself.
fixture_path = source / 'tests/run_oauth_login.py'
fixture = {'__file__': str(fixture_path)}
exec(compile(fixture_path.read_text().split("with tempfile.TemporaryDirectory(prefix='storage-oauth-')")[0],
             str(fixture_path), 'exec'), fixture)
compile_source, link = fixture['compile_source'], fixture['link']
runtime = fixture['runtime']


class Handler(fixture['Handler']):
    def cloud(self, method, body=b''):
        try:
            assert self.headers.get('Authorization', '').startswith('Bearer fixture-access-')
            path = urllib.parse.urlsplit(self.path).path
            value = json.loads(body) if body else {}
            self.server.cloud_calls.append((method, path))
            if path == '/resource/v3/projects:search':
                query = urllib.parse.parse_qs(urllib.parse.urlsplit(self.path).query)
                if 'pageToken' not in query:
                    return self.reply(200, {'projects': [
                        {'projectId': 'unrelated-project', 'displayName': 'storage-ai', 'state': 'ACTIVE'},
                        {'projectId': 'storage-ai-deleted', 'displayName': 'Deleted', 'state': 'DELETE_REQUESTED'}],
                        'nextPageToken': 'next-page'})
                assert query['pageToken'] == ['next-page']
                return self.reply(200, {'projects': list(self.server.projects.values())})
            if path == '/resource/v3/projects' and method == 'POST':
                assert value['displayName'] == 'My Project'
                assert re.fullmatch(r'storage-ai-[a-f0-9]{16}', value['projectId'])
                assert value['projectId'] not in self.server.projects
                self.server.projects[value['projectId']] = {**value, 'name': 'projects/123456', 'state': 'ACTIVE'}
                return self.reply(200, {'name': 'operations/project-create'})
            if path.startswith('/resource/v3/projects/'):
                project = path.rsplit('/', 1)[-1]
                return self.reply(200, self.server.projects[project]) if project in self.server.projects else self.reply(404, {})
            if path in ('/resource/v3/operations/project-create', '/services/v1/operations/enable'):
                return self.reply(200, {'done': True})
            if path == '/services/v1/projects/123456/services:batchEnable':
                assert value == {'serviceIds': ['drive.googleapis.com']}
                return self.reply(200, {'name': 'operations/enable'})
            if path.startswith('/iam/v1/projects/'):
                project = path.split('/')[4]
                if path.endswith('/serviceAccounts'):
                    if method == 'GET':
                        query = urllib.parse.parse_qs(urllib.parse.urlsplit(self.path).query)
                        if 'pageToken' not in query:
                            return self.reply(200, {'accounts': [{'email': 'external@'+project+'.iam.gserviceaccount.com'}],
                                                    'nextPageToken': 'service-page'})
                        assert query['pageToken'] == ['service-page']
                        return self.reply(200, {'accounts': [{'email': email} for email in self.server.service_accounts]})
                    assert value['accountId'] == 'my-service'
                    email = value['accountId']+'@'+project+'.iam.gserviceaccount.com'
                    self.server.service_accounts[email] = value['accountId']
                    return self.reply(200, {'email': email})
                if path.endswith('/keys'):
                    email = urllib.parse.unquote(path.split('/')[6])
                    credential = {'type': 'service_account', 'project_id': project, 'client_email': email,
                                  'private_key_id': 'fixture-key', 'private_key': 'synthetic-local-key'}
                    # Prevent reserve workers from contacting even the mock API.
                    queues = self.server.directory / 'blob_queue'
                    queues.mkdir(exist_ok=True)
                    (queues/hashlib.sha256(email.encode()).hexdigest()).write_text(json.dumps(['unused']*21000))
                    return self.reply(200, {'name': f'projects/{project}/serviceAccounts/{email}/keys/fixture-key',
                        'privateKeyData': fixture['base64'].b64encode(json.dumps(credential).encode()).decode()})
            raise AssertionError((method, path))
        except Exception:
            self.server.errors.append(fixture['traceback'].format_exc())
            self.reply(500, {})

    def do_GET(self):
        if urllib.parse.urlsplit(self.path).path.startswith('/drive/v3/files/'):
            return self.reply(200, {'name': json.dumps({'name': 'root', 'parent': 'null', 'parent_account': 'null',
                                                       'next': 'null', 'next_account': 'null'}), 'description': '[]'})
        return super().do_GET()

    def do_POST(self):
        if self.path.startswith('/drive/v3/files'):
            if self.headers.get('Transfer-Encoding') == 'chunked':
                data = b''
                while True:
                    count = int(self.rfile.readline().strip(), 16)
                    if not count: self.rfile.readline(); break
                    data += self.rfile.read(count); self.rfile.read(2)
            else:
                data = self.rfile.read(int(self.headers.get('Content-Length', 0)))
            assert 'name' in json.loads(data)
            self.server.created += 1
            return self.reply(200, {'id': 'gui-root-'+str(self.server.created)})
        return super().do_POST()


observation = r'''
        {
            auto rows=nlohmann::json::array(), footer=nlohmann::json::array();
            auto point=[](auto& object) {
                const auto p=object.get_pos(); const auto s=object.get_size();
                return nlohmann::json{{"x",p.x+s.x/2},{"y",p.y+s.y/2}};
            };
            std::function<void(lgo::GUI_object_t&)> inspect=[&](auto& object) {
                if (auto row=dynamic_cast<lgo::divx_soft_t*>(&object)) {
                    auto value=nlohmann::json::object(); value["actions"]=nlohmann::json::array();
                    row->visit_children([&](auto& child) {
                        if (auto text=dynamic_cast<lgo::text_line_scroll_std_t*>(&child)) value["text"]=text->get_text();
                        if (auto rect=dynamic_cast<lgo::rect_t*>(&child); rect && rect->get_size()==lgo::size_2D{32,32}) value["photo"]=true;
                        if (auto button=dynamic_cast<progress_button_t*>(&child)) {
                            auto at=point(*button); at["text"]=button->get_text(); value["actions"].push_back(at);
                        }
                        if (auto button=dynamic_cast<lgo::button_classic_std_t*>(&child)) {
                            auto at=point(*button);
                            std::function<void(lgo::GUI_object_t&)> label=[&](auto& object) {
                                if (auto text=dynamic_cast<lgo::text_line_t*>(&object)) at["text"]=text->get_text();
                                object.visit_children(label);
                            }; button->visit_children(label); footer.push_back(at);
                        }
                    });
                    if (value.contains("text")) rows.push_back(value);
                }
                object.visit_children(inspect);
            };
            if (persistent.m_account_menu) inspect(*persistent.m_account_menu->get_object_shared<lgo::divy_soft_t>(1));
            auto value=nlohmann::json{{"toolbar",point(*persistent.m_login_button)},
                {"label",persistent.m_login_button->get_text()},{"menu",bool(persistent.m_account_menu)},
                {"dialog",bool(persistent.m_dialog)},{"switching",persistent.m_switching},
                {"log_path",lgo::get_logfile_path()},{"workers",load_state.m_settings.worker_count},
                {"rows",rows},{"footer",footer},{"page",static_cast<int>(menu_page)},
                {"session",load_state.m_session ? load_state.m_session->storage_id : ""},
                {"root",load_state.m_session ? load_state.m_session->root : ""},
                {"status",persistent.m_directory_status->get_text()},
                {"window_w",engine.get_window_size().x},{"window_h",engine.get_window_size().y}};
            storage_atomic_write("spaces-ui.json",value.dump());
        }
'''

interaction = r'''
import json, subprocess, sys, time
from pathlib import Path
test=Path.cwd()
def command(*args):
    return subprocess.check_output(['xdotool',*map(str,args)],text=True).strip()
def snapshot(): return json.loads(Path('spaces-ui.json').read_text())
def wait(predicate, message, timeout=30):
    until=time.monotonic()+timeout
    while time.monotonic()<until:
        try:
            value=predicate()
            if value: return value
        except (OSError, json.JSONDecodeError): pass
        assert app.poll() is None, 'GUI exited early'
        time.sleep(.04)
    raise AssertionError(message+'; '+str(snapshot()))
def click(at): command('mousemove','--window',window,at['x'],at['y'],'click','1'); time.sleep(.12)
def ready(page):
    value=wait(lambda: snapshot() if snapshot()['menu'] and snapshot()['page']==page and not snapshot()['switching'] else None,'Missing menu '+str(page))
    assert value['log_path']==session_log,'Space operation split the session log'
    return value
def footer(label): click(next(item for item in snapshot()['footer'] if item['text']==label))
def action(label,row=0): click(next(item for item in snapshot()['rows'][row]['actions'] if item['text']==label))
def open_menu(): click(snapshot()['toolbar']); return ready(0)
def paste(text):
    subprocess.run(['wine64',sys.argv[2],text],check=True,stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL)
    command('windowfocus','--sync',window);command('key','ctrl+a');command('key','ctrl+v');time.sleep(.12)
def dialog(value, lines=0, accept=True):
    s=wait(lambda:snapshot() if snapshot()['dialog'] else None,'Missing dialog')
    height=152+28*lines+68
    left=(s['window_w']-720)//2; top=(s['window_h']-height)//2
    if value is not None:
        click({'x':left+200,'y':top+80+28*lines}); paste(value)
    click({'x':left+(600 if accept else 480),'y':top+height-48})
def closed(): wait(lambda:not snapshot()['dialog'],'Dialog stayed open')
def restart():
    global app,window,session_log
    command('key','alt+F4'); assert app.wait(timeout=15)==0
    Path('spaces-ui.json').unlink()
    app=subprocess.Popen(['wine64',sys.argv[1]])
    wait(lambda:Path('spaces-ui.json').exists(),'Restart failed')
    window=command('search','--onlyvisible','--name','wWinMain.engine').splitlines()[0]
    command('windowfocus','--sync',window)
    session_log=snapshot()['log_path']
app=subprocess.Popen(['wine64',sys.argv[1]])
try:
    wait(lambda:Path('spaces-ui.json').exists(),'GUI failed to start')
    window=command('search','--onlyvisible','--name','wWinMain.engine').splitlines()[0]
    command('windowfocus','--sync',window)
    assert snapshot()['label']=='空間' and not snapshot()['session']
    session_log=snapshot()['log_path']
    def logfile(value):return Path(value.replace('\\','/'))
    original_log=logfile(session_log)
    def emit(marker):
        Path('emit-log.next').write_text(marker)
        Path('emit-log.next').replace('emit-log')
        wait(lambda:marker in logfile(snapshot()['log_path']).read_text(),'Log marker was not written')
    def open_settings():
        Path('open-settings').touch()
        return wait(lambda:snapshot() if snapshot()['dialog'] else None,'Settings did not open')
    def settings_field(s,index,value):
        left=(s['window_w']-720)//2;top=(s['window_h']-692)//2
        click({'x':left+200,'y':top+124+68*index});paste(value)
    def finish_settings(s,accepted,workers=None):
        left=(s['window_w']-720)//2;top=(s['window_h']-692)//2
        click({'x':left+(640 if accepted else 520),'y':top+660})
        wait(lambda:not snapshot()['dialog'] and not snapshot()['switching'] and
             (workers is None or snapshot()['workers']==workers),'Settings did not finish')
        time.sleep(.2) # Include the deferred-menu turn after settings completion.
        assert not snapshot()['menu'],'Closing settings opened the space dropdown'
    emit('session-log-before-settings')
    initial_files=set(original_log.parent.glob('*.txt'))
    s=open_settings();finish_settings(s,False)
    assert not Path('settings.json').exists(),'Cancel unexpectedly saved settings'
    time.sleep(1.1) # Cross a filename timestamp boundary to detect log rotation.
    s=open_settings();settings_field(s,6,'17');finish_settings(s,True,17)
    assert snapshot()['log_path']==session_log and set(original_log.parent.glob('*.txt'))==initial_files
    emit('session-log-after-settings')
    open_menu();s=open_settings();finish_settings(s,False)
    s=open_settings();settings_field(s,0,'changed-logfile');finish_settings(s,True,17)
    changed_log=logfile(snapshot()['log_path'])
    assert changed_log.parent==Path('changed-logfile') and changed_log.name==original_log.name
    emit('session-log-in-changed-directory')
    assert 'session-log-before-settings' in original_log.read_text()
    s=open_settings();settings_field(s,0,'logfile');finish_settings(s,True,17)
    assert snapshot()['log_path']==session_log and set(original_log.parent.glob('*.txt'))==initial_files
    emit('session-log-returned-to-original')
    assert 'session-log-after-settings' in original_log.read_text()
    assert set(changed_log.parent.glob('*.txt'))=={changed_log}
    menu=open_menu();assert not menu['rows']
    footer('新增空間');dialog('');assert snapshot()['dialog'],'Empty space name accepted'
    dialog('My Space');closed();ready(0)
    settings=json.loads(Path('settings.json').read_text());space=settings['spaces'][0]
    assert space['name']=='My Space' and not space['signed_in'] and not space['accounts']
    assert [a['text'] for a in snapshot()['rows'][0]['actions']]==['登入','登出','編輯','移除']
    action('編輯');ready(1);footer('新增帳戶');ready(2)
    footer('新增使用者');ready(2)
    assert snapshot()['rows'][0]['text']=='oauth@example.invalid' and snapshot()['rows'][0]['photo']
    assert [a['text'] for a in snapshot()['rows'][0]['actions']]==['使用','移除']
    action('使用');ready(3);assert not snapshot()['rows'],'Project ID/state filtering failed'
    footer('新增專案');dialog('My Project');closed();ready(3)
    assert [r['text'] for r in snapshot()['rows']]==['My Project']
    action('使用');ready(4);assert not snapshot()['rows'],'External IAM account was listed'
    footer('新增帳戶');dialog('BAD');assert snapshot()['dialog'],'Invalid service name accepted'
    dialog('my-service');closed();ready(4)
    assert [r['text'] for r in snapshot()['rows']]==['my-service']
    action('使用');ready(1)
    assert [r['text'] for r in snapshot()['rows']]==['my-service']
    footer('返回');ready(0);action('登入');ready(0)
    wait(lambda:snapshot()['session'],'Space sign-in failed')
    root=snapshot()['root']; assert root
    restart();assert snapshot()['session']==space['id'] and snapshot()['root']==root
    open_menu();action('登出');ready(0);wait(lambda:not snapshot()['session'],'Space sign-out failed')
    restart();assert not snapshot()['session']
    open_menu();action('編輯');ready(1);action('移除')
    dialog('wrong',lines=5);assert snapshot()['dialog'],'Unlink skipped typed confirmation'
    dialog(None,lines=5,accept=False);closed();ready(1);assert snapshot()['rows']
    action('移除');dialog('移除關聯',lines=5);closed();ready(1)
    assert not snapshot()['rows'] and not json.loads(Path('settings.json').read_text())['spaces'][0]['accounts']
    assert list(Path('key').glob('*my-service.json')),'Unlink removed the key'
    # Relink, proving that unlink preserves both the managed key and root.
    footer('新增帳戶');ready(2);action('使用');ready(3);action('使用');ready(4);action('使用');ready(1)
    footer('返回');ready(0);action('登入');ready(0);assert snapshot()['root']==root
    # Inspect and cancel the all-account clear confirmation in the real GUI.
    command('key','Escape');wait(lambda:not snapshot()['menu'],'Menu did not close')
    # Toolbar "clear" is exposed separately by the instrumentation below.
    Path('open-clear').touch();wait(lambda:snapshot()['dialog'],'Missing clear warning')
    dialog(None,lines=5,accept=False);closed()
    open_menu();action('移除');dialog('wrong',lines=5);assert snapshot()['dialog']
    dialog(None,lines=5,accept=False);closed();ready(0);assert snapshot()['rows']
    action('移除');dialog('移除空間',lines=5);closed();ready(0)
    assert not snapshot()['rows'] and not (Path('root')/space['id']).exists()
    assert list(Path('key').glob('*my-service.json'))
    restart();open_menu();assert not snapshot()['rows']
    footer('新增空間');dialog('Temporary');closed();ready(0);action('編輯');ready(1)
    footer('新增帳戶');ready(2);action('移除');ready(2);assert not snapshot()['rows'],'Google user removal failed'
    command('key','Escape');command('key','alt+F4');assert app.wait(timeout=15)==0
    logs=''.join(path.read_text(errors='replace') for path in Path('logfile').glob('*.txt'))
    assert 'uncaught exception:' not in logs and 'finished_all!' in logs
    print('PASS: one log per process across settings/space operations, directory-change continuation, closed settings dropdown, rendered space/user/project/service menus, named provisioning, managed-key filtering, linking, login/logout/restart, typed removals, retained cloud identities and clear warning.')
finally:
    if app.poll() is None: app.kill();app.wait()
'''


with tempfile.TemporaryDirectory(prefix='storage-spaces-gui-') as directory:
    test = Path(directory)
    server = fixture['ThreadingHTTPServer'](('127.0.0.1', 0), Handler)
    server.host = 'http://127.0.0.1:'+str(server.server_port)
    server.directory = test
    server.issued = server.refreshes = server.created = 0
    server.grants, server.projects, server.service_accounts = {}, {}, {}
    server.identifiers, server.errors, server.cloud_calls = set(), [], []
    server.email = 'oauth@example.invalid'
    server.scope = fixture['cloud_scopes']
    threading.Thread(target=server.serve_forever, daemon=True).start()
    (test/'client.json').write_text(json.dumps({'installed': {'client_id': 'fixture.apps.googleusercontent.com',
        'client_secret': 'fixture-public-client-secret', 'token_uri': server.host+'/token', 'auth_uri': server.host+'/auth'}}))
    oauth = (source/'storage/google/oauth/.cpp').read_text().replace('module storage.google.oauth;',
        '#include "'+str(source/'tests/mock_oauth_cng.hpp')+'"\nmodule storage.google.oauth;')
    for original, replacement in [('https://oauth2.googleapis.com/token', server.host+'/token'),
        ('https://accounts.google.com/o/oauth2/v2/auth', server.host+'/auth'),
        ('https://www.googleapis.com/oauth2/v2/userinfo', server.host+'/oauth2/v2/userinfo')]:
        oauth = oauth.replace(original, replacement)
    cloud = (source/'storage/google/cloud/.cpp').read_text()
    for host, route in [('cloudresourcemanager', 'resource'), ('serviceusage', 'services'), ('iam', 'iam')]:
        oauth = oauth.replace('https://'+host+'.googleapis.com/', server.host+'/'+route+'/')
        cloud = cloud.replace('https://'+host+'.googleapis.com/', server.host+'/'+route+'/')
    oauth = oauth.replace('CURLOPT_PROTOCOLS_STR,"https"', 'CURLOPT_PROTOCOLS_STR,"http,https"')
    oauth = oauth.replace('if (!m_impl->photo.starts_with("https://"))', 'if (!m_impl->photo.starts_with("http://127.0.0.1:"))')
    oauth = oauth.replace('L"LOCALAPPDATA"', 'L"STORAGE_OAUTH_TEST_VAULT"')
    (test/'oauth.cpp').write_text(oauth)
    (test/'cloud.cpp').write_text(cloud)
    entry = (source/'storage/storage.cpp').read_text()
    entry = entry.replace('        lgo::text_line_t::pending_load();', observation+'''
        if (std::filesystem::exists("open-clear")) {
            std::filesystem::remove("open-clear"); load_state.m_clear_page();
        }
        if (std::filesystem::exists("open-settings")) {
            std::filesystem::remove("open-settings"); load_state.m_settings_page();
        }
        if (std::filesystem::exists("emit-log")) {
            lgo::log_file(storage_read("emit-log")+"\\n"); std::filesystem::remove("emit-log");
        }
        lgo::text_line_t::pending_load();''')
    (test/'entry.cpp').write_text(entry)
    # Share the existing native Windows Unicode clipboard helper.
    clipboard = (source/'tests/run_gui_settings.py').read_text().split("clipboard.write_text(r'''")[1].split("''')")[0]
    (test/'clipboard.cpp').write_text(clipboard)
    watcher_stop = threading.Event()
    def browser():
        path = test/'browser-url'
        while not watcher_stop.wait(.03):
            if not path.exists(): continue
            url = path.read_text(); path.unlink()
            query = urllib.parse.parse_qs(urllib.parse.urlsplit(url).query)
            server.challenge = query['code_challenge'][0]
            server.redirect = query['redirect_uri'][0]
            assert query['scope'] == [fixture['cloud_scopes']]
            urllib.request.urlopen(server.redirect+'?code=fixture-code&state='+query['state'][0], timeout=10).close()
    threading.Thread(target=browser, daemon=True).start()
    try:
        for original, replacement, output in [
            ('storage/google/easy/.cpp', source/'tests/mock_google_easy.cpp', 'easy.obj'),
            ('storage/google/oauth/.cpp', test/'oauth.cpp', 'oauth.obj'),
            ('storage/google/cloud/.cpp', test/'cloud.cpp', 'cloud.obj'),
            ('storage/storage.cpp', test/'entry.cpp', 'entry.obj')]:
            compile_source(original, replacement, test/output)
        link(test/'gui.exe', test/'entry.obj', test/'oauth.obj', test/'easy.obj', gui=True, cloud=test/'cloud.obj')
        subprocess.run(['x86_64-w64-mingw32-clang++', '-municode', str(test/'clipboard.cpp'), '-o', str(test/'clipboard.exe')], check=True)
        for dll in (runtime/'bin').glob('*.dll'): shutil.copy2(dll, test/dll.name)
        for resource in ('font', 'shader'): shutil.copytree(runtime/resource, test/resource)
        (test/'vault').mkdir()
        (test/'key').mkdir()
        (test/'key/external.json').write_text('{"client_email":"external@example.invalid","private_key":"ignore"}')
        env = {**os.environ, 'WINEDEBUG': '-all', 'STORAGE_GPU_TEST_PORT': str(server.server_port),
            'STORAGE_OAUTH_TEST_VAULT': 'Z:'+str(test/'vault').replace('/', '\\')}
        subprocess.run(['xvfb-run', '-a', '-s', '-screen 0 1600x1200x24', 'python3', '-c', interaction,
                        str(test/'gui.exe'), str(test/'clipboard.exe')], cwd=test, env=env, check=True, timeout=200)
        assert not server.errors, server.errors
        assert sum(method=='POST' and path=='/resource/v3/projects' for method,path in server.cloud_calls)==1
        assert sum(method=='POST' and path.endswith('/serviceAccounts') for method,path in server.cloud_calls)==1
        assert not any(method=='DELETE' for method,path in server.cloud_calls), 'Local removal deleted Cloud resources'
    except BaseException:
        evidence = Path('/tmp/storage-spaces-gui-failure')
        shutil.rmtree(evidence, ignore_errors=True); shutil.copytree(test, evidence)
        print('Synthetic GUI evidence:', evidence, 'Fixture errors:', server.errors, flush=True)
        raise
    finally:
        watcher_stop.set(); server.shutdown(); server.server_close()
