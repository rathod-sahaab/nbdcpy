# Bugs

Current bugs in the program.

### URING: all header reads run before one can be processed completely

When a response to read request is recieved, all `read_header` operations are ran as socket is readable. This causes Data to be read as header.
Instead of reading

```
HEADER1 + DATA...
```

Data is read as header of other requests

```
HEADER1 + HEADER2 + HEADER3 + HEADER4 ...
```

Hence other header reads are errorneous.

#### Fixes

##### io_uring link feature

`io_uring` has a feature called link, we can link commands so that one is only ran after previous request was complete, but it has to be studied further.

status: **Doesn't work**

reason: link feature only promises that an operation is ran after previous has been completed not when it's seen.

##### Only add read request after we see a cqe

Currently, we batch up `MAX_INFLIGHT_REQUEST` number of write requests, followed by equal number of `read_header_from_socket` requests. This causes uring to read just headers from socket at once, i.e. from data part of first response.

1. The idea is to submit only one uring `read_header_from_socket` request
2. reading data (main and fallback)
  i. If there is no error(NBD error) on completetion we enqueue a `read_data_from_socket` request to uring and immediately call `io_uring_submit_and_wait()` to get it's response. (this part won't work if did as described)
  ii. `recv` directly from the socket.
3. Queue another `read_header_from_socket` request
4. Call `io_uring_cqe_seen(cqe)`

So at any time there will be only on `read_header_from_socket` request in `io_uring`.

status: **In development.**

