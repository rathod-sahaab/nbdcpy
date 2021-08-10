# Bugs

Current bugs in the program.

### URING: all header reads run before one can be processed completely [SOLVED]

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

- [ ] If there is no error(NBD error) on completetion we enqueue a `read_data_from_socket` request to uring and immediately call `io_uring_submit_and_wait()` to get it's response. (this part won't work if did as described and need to find actual approach)
- [x] `recv` directly from the socket.

3. Queue another `read_header_from_socket` request
4. Call `io_uring_cqe_seen(cqe)`

So at any time there will be only on `read_header_from_socket` request in `io_uring`.

status: **WORKS**

### nbdcpy stops after copying random amount of data

Copying the disk stops at random points (sometimes full copy operation executed)

#### Potential causes

- related to `read_request_surplus` variable that track how many read requests have no associated `read_header` in `io_uring`. This variable assists so that we don't request socket for more headers than it can provide.

- related `inqueue_read_header_reqs` variable that track how many header reads are enqueued in `io_uring` (there should always be atleast and atmost one).

- related to waiting state logic, Operation has 4 state `REQUESTING, READING, WRITING, CONFIRMING` but as only one read can be queued(for multiplexing) we got and additional state `WAITING` making the new order as `REQUESTING, WAITING, READING, WRITING, CONFIRMING`. For an operation after requesting some data the source nbd server for an opeation we change it's state to waiting, then when we read some header from source socket and find it to be for handle `H` we switch the state to `READING` then read data and move move to writing and queue another read header which when completed will move another operation forward from waiting.

#### LOG

This log below might help

```
w w r w w w w w C w w w w w w w
Read request surplus: 15, inqueue_read_header_reqs: 1
Operation States:
w w w w w w w w C w w w w w w w
Read request surplus: 16, inqueue_read_header_reqs: 1
CQE uring user data is read
Reply handle: 8
Sending read request to 'source' for 512 bytes at offset 2379264 and handle [8]
Operation States:
nbdkit: random.6w w w w w w w w r w w w w w w w
: Read request surplus: 16, inqueue_read_header_reqs: 1
debug: Operation States:
random: pread count=512 offset=2378752
w w w w w w w w w w w w w w w w
Read request surplus: 17, inqueue_read_header_reqs: 1
nbdkit: random.2: debug: random: pread count=512 offset=2379264
```

Here we have 16 operation running represented by 16 space seperated letters.

```yaml
REQUESTING: 'r'
WAITING: 'w';
READING: 'R';
WRITING: 'W';
CONFIRMING: 'C';
EMPTY: 'E';
```

Also this only happens with large size small size `<1M` completes seamlessly.

** ANALYSIS **

- `read_request_surplus` should not go above 16 but I don't think that should affect much.
