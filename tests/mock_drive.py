import base64,json,pathlib,re,threading,time,urllib.parse
from http.server import ThreadingHTTPServer,BaseHTTPRequestHandler
base=pathlib.Path.cwd()
lock=threading.RLock()
blobs={k:{'name':json.dumps({'name':k,'parent':'null','next':'null'}),'description':'[]'} for k in ('root','other')}
fixture=base/'download-fixture.json'
if fixture.exists(): blobs.update(json.loads(fixture.read_text()))
fail_content=True
fail_publish=True
fail_transport=True
violations=[]
events=[]
def decode_file(id):
    result=b''
    while id!='null':
        blob=blobs[id]; name=json.loads(blob['name'])
        for chunk in json.loads(blob['description']):
            for content in json.loads(blobs[chunk]['description']):
                text=blobs[content]['description']; result+=base64.urlsafe_b64decode(text+'='*((-len(text))%4))
        id=name['next']
    return result

def verify(entity,local):
    id=entity['id']; blob=blobs[id]; name=json.loads(blob['name'])
    assert entity['name']==local.name,(entity['name'],local.name)
    if entity['type']=='file':
        assert name['size']==local.stat().st_size
        assert decode_file(id)==local.read_bytes(),f'wrong or incomplete file: {local}'
    else:
        children=[]
        while id!='null':
            blob=blobs[id]; name=json.loads(blob['name']);children+=json.loads(blob['description']); id=name['next']
        assert {v['name'] for v in children}=={v.name for v in local.iterdir()}
        for child in children: verify(child,local/child['name'])
class Handler(BaseHTTPRequestHandler):
    protocol_version='HTTP/1.1'
    def body(self):
        if self.headers.get('Transfer-Encoding')=='chunked':
            result=b''
            while True:
                size=int(self.rfile.readline().strip().split(b';')[0],16)
                if size==0:
                    self.rfile.readline(); return result
                result+=self.rfile.read(size); self.rfile.read(2)
        return self.rfile.read(int(self.headers['Content-Length']))
    def log_message(self,*args): pass
    def respond(self,code,data,ctype='application/json'):
        if not isinstance(data,bytes): data=json.dumps(data).encode()
        self.send_response(code);self.send_header('Content-Type',ctype);self.send_header('Content-Length',str(len(data)));self.end_headers();self.wfile.write(data)
    def do_GET(self):
        if urllib.parse.urlparse(self.path).path.rsplit('/',1)[-1].startswith('slow-'): time.sleep(.15)
        with lock:
            if self.path=='/report':
                return self.respond(200,{'violations':violations,'events':events,'root':json.loads(blobs['root']['description']),'fail_content_consumed':not fail_content,'fail_publish_consumed':not fail_publish})
            id=urllib.parse.urlparse(self.path).path.rsplit('/',1)[-1]
            events.append(['GET',id])
            self.respond(200,blobs.get(id,{}))
    def patch(self,id,data):
        global fail_content,fail_publish
        with lock:
            if id=='root':
                for child in json.loads(data['description']):
                    try: verify(child,base/'input'/child['name'])
                    except Exception as e: violations.append(str(e)); return 400,{'error':str(e)}
                if fail_publish:
                    fail_publish=False; events.append(['RETRY_PUBLICATION',id]); return 503,{'error':'fixture publication retry'}
            elif 'description' in data and not data['description'].startswith('[') and fail_content:
                fail_content=False; events.append(['RETRY_CONTENT',id]); return 503,{'error':'fixture content retry'}
            blobs.setdefault(id,{}).update(data)
            events.append(['PATCH',id])
            return 200,blobs[id]
    def do_PATCH(self):
        global fail_transport
        data=json.loads(self.body())
        id=urllib.parse.urlparse(self.path).path.rsplit('/',1)[-1]
        if id=='root' and not fail_publish and fail_transport:
            fail_transport=False; events.append(['RETRY_TRANSPORT',id]); self.close_connection=True; return
        # Keep one response in flight after metadata requests have already started.
        if id=='blob-3': time.sleep(0.15)
        self.respond(*self.patch(id,data))
    def do_POST(self):
        body=self.body().decode()
        boundary=self.headers['Content-Type'].split('boundary=')[1]
        parts=[]
        for part in body.split('--'+boundary)[1:-1]:
            header,request=part.lstrip('\r\n').split('\r\n\r\n',1)
            content_id=re.search(r'Content-ID: (\d+)',header)[1]
            first=request.split('\r\n',1)[0]
            id=urllib.parse.urlparse(first.split()[1]).path.rsplit('/',1)[-1]
            if first.startswith('GET '):
                code,response=200,blobs[id]
                events.append(['BATCH_GET',id])
            else:
                data=json.loads(request.split('\r\n\r\n',1)[1].strip())
                code,response=self.patch(id,data)
            encoded=json.dumps(response)
            parts.append(f'--fixture\r\nContent-Type: application/http\r\nContent-ID: response-{content_id}\r\n\r\nHTTP/1.1 {code} OK\r\nContent-Type: application/json\r\n\r\n{encoded}\r\n')
        response=(''.join(parts)+'--fixture--\r\n').encode()
        self.respond(200,response,'multipart/mixed; boundary=fixture')
server=ThreadingHTTPServer(('127.0.0.1',0),Handler)
(base/'server-port').write_text(str(server.server_port))
server.serve_forever()
