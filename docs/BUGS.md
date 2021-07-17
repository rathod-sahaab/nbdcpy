# Bugs

Current bugs in the program.

### URING header reads run before one can be processed completely

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

`io_uring` has a feature called link, we can link commands so that one is only ran after previous request was complete, but it has to be studied further.
