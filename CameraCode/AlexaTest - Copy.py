import logging
import os

from flask import Flask
from flask_ask import Ask, request, session, question, statement


app = Flask(__name__)
ask = Ask(app, "/")
logging.getLogger('flask_ask').setLevel(logging.DEBUG)


@ask.intent('changeproblem', mapping = {'problem_number':'problem_number'})
def change_problem(problem_number):
    speech_text="Setting Problem " + str(problem_number)
    return statement(speech_text).simple_card('Hello',speech_text)



if __name__ == '__main__':
    if 'ASK_VERIFY_REQUESTS' in os.environ:
        verify = str(os.environ.get('ASK_VERIFY_REQUESTS', '')).lower()
        if verify == 'false':
            app.config['ASK_VERIFY_REQUESTS'] = False
    app.run(debug=True)
