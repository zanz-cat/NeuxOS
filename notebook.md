### osx上编译安装bochs
```
brew install sdl2

./configure --enable-debugger --enable-disasm --with-sdl2
make && make install
```