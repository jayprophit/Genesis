"""Standalone localhost NumPy inference. No torch import, no external model API."""
from __future__ import annotations
import argparse, json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from threading import Lock
import numpy as np
from .native_runtime import GenesisNative

COLORS={'red':(1.,0.,0.),'green':(0.,1.,0.),'blue':(0.,0.,1.)}
SHAPES=('square','circle')

def toy_image(color:str,shape:str,seed:int)->np.ndarray:
    rng=np.random.default_rng(seed)
    yy,xx=np.mgrid[:16,:16];cx,cy=8+rng.uniform(-1.1,1.1),8+rng.uniform(-1.1,1.1)
    r=4.6+rng.uniform(-0.4,0.4)
    if shape=='circle': mask=(xx-cx)**2+(yy-cy)**2 <= r*r
    else: mask=(np.abs(xx-cx)<=r)&(np.abs(yy-cy)<=r)
    image=np.zeros((3,16,16),dtype=np.float32)
    image[:,mask]=np.asarray(COLORS[color],dtype=np.float32)[:,None]
    return np.clip(image+rng.uniform(0,0.02,image.shape).astype(np.float32),0,1)

def make_handler(model):
    lock=Lock()
    class NativeHandler(BaseHTTPRequestHandler):
        server_version='GenesisNative/0.2'
        def respond(self,code,obj):
            blob=json.dumps(obj).encode('utf-8')
            self.send_response(code);self.send_header('Content-Type','application/json; charset=utf-8')
            self.send_header('Cache-Control','no-store');self.send_header('Content-Length',str(len(blob)))
            self.end_headers();self.wfile.write(blob)
        def do_GET(self):
            if self.path!='/health':return self.respond(404,{'error':'not_found'})
            return self.respond(200,{'status':'running','engine':'genesis_native_numpy_v0.2',
                'weights':'first_party_original','model':'GenesisSeed','backend':'numpy_cpu',
                'scope':'toy_text_and_synthetic_vision','agent_tools':'disabled','camera':'disabled'})
        def do_POST(self):
            if self.path not in ('/v1/text/generate','/v1/vision/toy'):
                return self.respond(404,{'error':'not_found'})
            try:
                n=int(self.headers.get('Content-Length','0'))
                if n<1 or n>8192:return self.respond(413,{'error':'invalid_request_size'})
                request=json.loads(self.rfile.read(n))
                if not isinstance(request,dict):return self.respond(400,{'error':'json_object_required'})
                tokens=request.get('max_new_tokens',32)
                if type(tokens)!=int or not 1<=tokens<=80:return self.respond(400,{'error':'invalid_token_limit'})
                with lock:
                    if self.path=='/v1/text/generate':
                        prompt=request.get('prompt','')
                        if not isinstance(prompt,str) or len(prompt.encode('utf8'))>80:
                            return self.respond(400,{'error':'invalid_prompt'})
                        response=model.generate(prompt=prompt,max_new_tokens=tokens)
                        return self.respond(200,{'response':response,'scope':'toy_text_only'})
                    color=request.get('color','red'); shape=request.get('shape','square')
                    seed=request.get('seed',999)
                    if color not in COLORS or shape not in SHAPES or type(seed)!=int or not 0<=seed<100000000:
                        return self.respond(400,{'error':'invalid_shape_or_seed'})
                    response=model.generate(image=toy_image(color,shape,seed),max_new_tokens=tokens)
                    return self.respond(200,{'response':response,'target':color+' '+shape,'scope':'synthetic_only'})
            except (ValueError,TypeError,json.JSONDecodeError):
                return self.respond(400,{'error':'invalid_request'})
            except Exception:
                return self.respond(500,{'error':'inference_failed'})
    return NativeHandler

def main():
    p=argparse.ArgumentParser();p.add_argument('--model',default='native_model');p.add_argument('--port',type=int,default=8766)
    a=p.parse_args();engine=GenesisNative(a.model)
    service=ThreadingHTTPServer(('127.0.0.1',a.port),make_handler(engine))
    print(f'Genesis independent CPU inference: http://127.0.0.1:{a.port}/health',flush=True)
    try: service.serve_forever()
    finally: service.server_close()
if __name__=='__main__':main()
