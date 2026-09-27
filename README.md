# 0x859  
### Assumptions:
 - Works with one producer and one consumer.   
 - Fixed capacity chosen at construction time.  
 - New elements are not added to the queue when it’s full.  
 - No blocking operations (e.g. pop_wait()) — handle synchronization yourself.  

### Known limitations:  
 - Works with one producer and one consumer.  
 - When head/tail counters overflow, it’s GG.  

### How to build:  
```
cmake -S . -B build
cmake --build build
```

### How to run the tests:  
```
ctest --test-dir build --output-on-failure
```

#### Author notes:
Works on my machine.  