### osx上编译安装bochs
```
brew install sdl2

./configure --enable-debugger --enable-disasm --with-sdl2
make && make install
```

### Windows安装x86_64-elf-ld(???)
```
wget https://mirrors.tuna.tsinghua.edu.cn/gnu/binutils/binutils-2.36.1.tar.xz

./configure --target=x86_64-elf --program-prefix=x86_64-elf
```