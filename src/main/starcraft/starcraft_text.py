"""
Subclasses text_agent.
------
See LICENSE.txt for licensing information.
------
"""

from nluas.language.text_agent import *
import sys

class StarcraftTextAgent(TextAgent):

    def __init__(self, args):
        TextAgent.__init__(self, args)
        self.solve_destination = "{}_{}".format(self.federation, "ProblemSolver")
        self.transport.subscribe(self.solve_destination, self.solver_callback)


    def callback(self, ntuple):
        """ Callback for receiving information from UI-Agent. """
        #ntuple = json.loads(ntuple)
        print(ntuple)
        if "type" in ntuple and ntuple['type'] == "clarification":
            self.clarification = True
            self.original = ntuple['original']
            self.output_stream(ntuple['tag'], ntuple['message'])

    def solver_callback(self, ntuple):
        ntuple = json.loads(ntuple)
        command = {'text': ntuple["command"], 'type': "standard"}
        self.transport.send(self.ui_destination, command)



if __name__ == "__main__":
    text = StarcraftTextAgent(sys.argv[1:])
    text.keep_alive(text.prompt)
