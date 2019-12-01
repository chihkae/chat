Given that the parsing code provided is buggy and the order in which options are given matters, please run with the -v
(debugging option) and see if it's the parsing that wrong before failing the tests for the server/client. It is possible
that reordering the options could make things work normally, given that I have run all tests on the rubric and should work
most cases.

Another thing to consider when grading is that, sometimes the server won't work if the same port is used over and over again.
Simply changing the port will most likely result in binding correctly. The same applies for clients. Please consider these
points when grading. Thank you!