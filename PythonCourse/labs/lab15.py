import time


class Timer:
#    def __init__(self):


    def __enter__(self):
        self.start = time.time()
        return self.start

    def __exit__(self, type, value, traceback):
        duration = time.time() - self.start
        print duration
        return True


def do_smf():
    time.sleep(3)


with Timer():
    do_smf()
