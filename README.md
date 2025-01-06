<H1 style="text-align: center;"> CS3.304: Advanced Operating Systems </H1>
<H3 style="text-align: center;"> Assignment 3: Peer-to-Peer File Sharing </H3>


### Description

This assignment is an implementation of a peer-to-peer distributed filesystem using group-based tracker and peers architecture. The following commands, when executed from the client, are currently supported:

```
1. create_user <user_id> <password>
2. login <user_id> <password>
3. create_group <group_id>
4. join_group <group_id>
5. leave_group <group_id>
6. list_requests <group_id>
7. accept_request <group_id> <user_id>
8. list_groups
9. list_files <group_id>
10. upload_file <file_path> <group_id>
11. download_file <group_id> <file_path> <destination_path>
12. stop_sharing <group_id> <file_path>
13. show_downloads
14. logout
```

### How to Execute:

1. Compile the source code using the Makefile:
    ```
    aashrjai@aashrey-tuf: ~/<path>/20224202012_A3$ make
    ```

2. Create the `tracker_info.txt` file containing the socket details for the tracker:
    ```
    aashrjai@aashrey-tuf: ~/<path>/20224202012_A3$ vi tracker_info.txt
    aashrjai@aashrey-tuf: ~/<path>/20224202012_A3$ cat tracker_info.txt
    127.0.0.1:5000
    ```

3. Start the tracker:
    ```
    aashrjai@aashrey-tuf: ~/<path>/20224202012_A3$ ./tracker.out tracker_info.txt <tracker_index>
    ```

4. Start the client/peer:
    ```
    aashrjai@aashrey-tuf: ~/<path>/20224202012_A3$ ./client.out <IP>:<PORT> tracker_info.txt
    ```

5. Start executing commands from the client!

### How to Exit Gracefully:

1. Terminate the connection with the tracker from the client by sending the `logout` command.

2. Terminate the client by using the `quit` command.

3. Terminate the tracker by using the `quit` command.

The tracker will only terminate when all the clients have terminated.
