macro define offsetof(_type, _mb) \
    ((long)(&((_type *)0)->_mb))

macro define container_of(_ptr, _type, _mb) \
    ((_type *)((void *)(_ptr) - offsetof(_type, _mb)))