"""Original local inference HTTP service. Deliberately tiny; not Ollama-compatible.
Loopback-only read/infer endpoints. No externally exposed camera/tool execution.
"""
from __future__ import annotations
import argparse
import json
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from threading import Lock
from .train import load_checkpoint
from .data import COLORS,SHAPES,synthetic_image


def make_handler(model):
    inference_lock = Lock()

    class GenesisHandler(BaseHTTPRequestHandler):
        server_version='AetheriusSeed/0.1'

        def _send(self, status:int, data:dict):
            body=json.dumps(data).encode('utf-8')
            self.send_response(status)
            self.send_header('Content-Type','application/json; charset=utf-8')
            self.send_header('Content-Length',str(len(body)))
            self.send_header('Cache-Control','no-store')
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            if self.path!='/health':
                return self._send(404,{'error':'Unknown route'})
            self._send(200,{'status':'running','model':'first_party_genesis_seed_v0.1',
                            'weights':'trained_from_random_initialization',
                            'supported':['toy_text_generation','toy_image_caption'],
                            'camera_api':'disabled','tool_execution':'disabled',
                            'native_neural_tts':'not_implemented',
                            'neural_renderer':'not_implemented'})

        def do_POST(self):
            if self.path not in ('/v1/text/generate','/v1/vision/toy'):
                return self._send(404,{'error':'Unknown route'})
            try:
                length=int(self.headers.get('Content-Length','0'))
                if not 0<length<8192:
                    return self._send(413,{'error':'Invalid request size'})
                args=json.loads(self.rfile.read(length))
                if not isinstance(args,dict):
                    return self._send(400,{'error':'Expected JSON object'})
                with inference_lock:
                    if self.path=='/v1/text/generate':
                        prompt=args.get('prompt','')
                        if not isinstance(prompt,str) or len(prompt.encode('utf-8'))>80:
                            return self._send(400,{'error':'prompt too long or invalid'})
                        count=args.get('max_new_tokens',48)
                        if not isinstance(count,int) or not 1<=count<=80:
                            return self._send(400,{'error':'invalid max_new_tokens'})
                        response=model.generate(prompt=prompt,max_new_tokens=count)
                        return self._send(200,{'response':response,'model':'genesis_seed',
                                               'scope':'toy_data_only'})
                    color=args.get('color','red')
                    shape=args.get('shape','square')
                    if color not in COLORS or shape not in SHAPES:
                        return self._send(400,{'error':'only red/green/blue and square/circle supported'})
                    seed=args.get('seed',999)
                    if not isinstance(seed,int) or not 0<=seed<100000000:
                        return self._send(400,{'error':'invalid seed'})
                    frame=synthetic_image(color,shape,seed=seed)[None]
                    response=model.generate(image=frame,max_new_tokens=32)
                    return self._send(200,{'response':response,'target':f'{color} {shape}',
                                           'scope':'synthetic_shape_only'})
            except (ValueError,TypeError,json.JSONDecodeError) as e:
                return self._send(400,{'error':'malformed request'})
            except Exception:
                return self._send(500,{'error':'internal inference error'})

    return GenesisHandler


def main():
    p=argparse.ArgumentParser()
    p.add_argument('--checkpoint',default='genesis_seed.pt')
    p.add_argument('--port',type=int,default=8765)
    args=p.parse_args()
    model=load_checkpoint(args.checkpoint)
    server=ThreadingHTTPServer(('127.0.0.1',args.port),make_handler(model))
    print('Genesis Seed API http://127.0.0.1:%s/health (loopback only)'%args.port,flush=True)
    try:server.serve_forever()
    finally:server.server_close()
if __name__=='__main__':main()
