"""Synthetic Genesis agent core (fixture only)."""


def run_task(name):
    return "ran " + name


def version():
    return "0.0.1-synthetic"


def health():
    return "ready"


def get_status():
    return dict(status='ready', mode='build')