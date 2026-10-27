"""Explicit, local-only, single-frame camera acquisition; no webcam connection here."""
from __future__ import annotations
import numpy as np
import torch


def frame_to_image(frame_bgr: np.ndarray, target_size:int=16) -> torch.Tensor:
    try:
        import cv2
    except ImportError as e:
        raise RuntimeError('Install opencv-python for local camera support') from e
    rgb=cv2.cvtColor(cv2.resize(frame_bgr,(target_size,target_size)),cv2.COLOR_BGR2RGB)
    return torch.from_numpy(rgb.copy()).permute(2,0,1).float().div(255.)[None]


def capture_frame(camera_index:int=0, *, user_authorized:bool=False) -> np.ndarray:
    if not user_authorized:
        raise PermissionError('Camera access denied by default; explicit --enable-camera required')
    try:
        import cv2
    except ImportError as e:
        raise RuntimeError('Install opencv-python for local camera support') from e
    camera=cv2.VideoCapture(camera_index)
    try:
        ok,frame=camera.read()
        if not ok:
            raise RuntimeError('Camera unavailable or OS privacy permission denied')
        return frame
    finally:
        camera.release()
