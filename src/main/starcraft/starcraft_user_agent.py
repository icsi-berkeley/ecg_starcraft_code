"""
Author: seantrott <seantrott@icsi.berkeley.edu>
Similar to a regular UserAgent, but it uses a StarcraftSpecializer instead.
"""

from nluas.language.user_agent import *
from starcraft_specializer import StarcraftSpecializer

class StarcraftUserAgent(UserAgent):

    def __init__(self, args):
        UserAgent.__init__(self, args)
        self._terminate = False

    def initialize_specializer(self):
        self.specializer = StarcraftSpecializer(self.analyzer)

    def output_stream(self, tag, message):
        print("{}: {}".format(tag, message))

if __name__ == "__main__":
    ui = StarcraftUserAgent(sys.argv[1:])
    ui.keep_alive()
