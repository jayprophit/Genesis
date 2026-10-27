import json
import unittest
import threading
from http.server import ThreadingHTTPServer
from urllib.request import Request, urlopen
from urllib.error import HTTPError
from pathlib import Path
from genesis_seed.native_runtime import GenesisNative
from genesis_seed.server_native import make_handler

class NativeHTTPTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source=Path(__file__).resolve().parents[1]/'models/native_visual_seed'
        if not (source/'manifest.json').exists():
            raise unittest.SkipTest('Optional exported demonstration model not found')
        cls.model=GenesisNative(source)
        cls.server=ThreadingHTTPServer(('127.0.0.1',0),make_handler(cls.model))
        cls.thread=threading.Thread(target=cls.server.serve_forever,daemon=True);cls.thread.start()
        cls.url=f'http://127.0.0.1:{cls.server.server_port}'
    @classmethod
    def tearDownClass(cls):
        if hasattr(cls,'server'):
            cls.server.shutdown();cls.server.server_close();cls.thread.join(timeout=2)
    def post(self,url,data):
        req=Request(self.url+url,method='POST',data=json.dumps(data).encode(),headers={'Content-Type':'application/json'})
        with urlopen(req,timeout=20) as resp:return resp.status,json.load(resp)
    def test_health(self):
        with urlopen(self.url+'/health',timeout=10) as response:
            value=json.load(response)
        self.assertEqual(value['backend'],'numpy_cpu')
        self.assertEqual(value['camera'],'disabled')
    def test_generate(self):
        code,value=self.post('/v1/text/generate',{'prompt':'genesis','max_new_tokens':10})
        self.assertEqual(code,200)
        self.assertIsInstance(value['response'],str)
    def test_synthetic_vision(self):
        code,value=self.post('/v1/vision/toy',{'color':'red','shape':'square','seed':923})
        self.assertEqual(value['response'],'red square')
    def test_bad_prompt(self):
        with self.assertRaises(HTTPError) as e:
            self.post('/v1/text/generate',{'prompt':'x'*90})
        self.assertEqual(e.exception.code,400)

if __name__=='__main__':unittest.main()
