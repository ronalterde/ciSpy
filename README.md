# CI Spy
Show status of Continuous Integration server (Jenkins) using an RGB LED and a beeper.

## Initialize
```
git submodule update --init
```

## How to build for host
```
export CXX=g++-5
export CC=gcc-5
make all
make test
```

## How to build for target
```
source /opt/yocto.../environment-setup....
make all
```


