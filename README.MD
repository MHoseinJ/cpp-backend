# CPP WEB SERVER PROJECT

this project is a really lightweight web server that can also handle some basic logics like routing and scripting and etc

## installation
```shell
# install lua (archlinux)
sudo pacman -S lua

# install lua for (fedora linux)
sudo dnf install lua

# install sol2
git clone https://github.com/ThePhD/sol2.git &&
cd sol2 &&
mkdir build &&
cd build &&
cmake .. &&
make &&
sudo make install

# clone the project
git clone https://github.com/MHoseinJ/cpp-backend.git
cd cpp-backend
mkdir build && cd build
cmake ..
make
```
you are good to go! just run the engine!
```shell
./backend
```

# Usage
unfortunately still there is no documentation except reading the source, but I'll add it later!

# special thanks to [ThePhD](https://github.com/ThePhD) and [nlohmann](https://github.com/nlohmann)