1.Given that the parsing code provided is buggy and the order in which options are given matters, please run with the -v
(debugging option) and see if it's the parsing that wrong before failing the tests for the server/client. It is possible
that reordering the options could make things work normally, given that I have run all tests on the rubric and should work
most cases.

2. Another thing to consider when grading is that, sometimes the server won't work if the same port is used over and over again.
Simply changing the port will most likely result in binding correctly. The same applies for clients.

3. In terms of client, for hosts I have only considered localhost as there is no way for me to mimic google.com or other
ip addresses. For servers, it will only work on local host as well.


