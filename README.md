Just a simple chat using the terminal for communicating

First of all server must be compiled and ran, after any of other terminals can compile and run the client file and after that start communicating

Features of the chat:
1) Rooms for chatting, people at the same server can be at different rooms so that they will not see each other's messages, each room is individual
2) Messages can be written to all members of the room same as to a particular group of members, so private messages are available
3) Handling of the problem of the same nicknames (e.x. if there is "Nick", and tyou want to register as "Nick" it will give you username "Nick_1", if it is not available too, it will give "Nick_2", etc.)

Commands:
1) "/quit" - quit the chat
2) "/join x" - join the room x, where x is the number of room
3) "/list" - list the members of the room
4) "All : MESSAGE" - sends message for all members of the room
5) "nickname1, nickname2, nickname3, ... : MESSAGE" - sends message for the given list of members of the room
