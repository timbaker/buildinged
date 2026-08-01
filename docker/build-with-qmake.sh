#sudo docker run --rm -v "$(pwd)/../..":/workspace qt-builder \
#    bash -c "ls /opt/qt/6.11.1"

sudo docker run --rm -v "$(pwd)/../..":/workspace qt-builder \
    bash -c "mkdir -p build-buildinged-docker && cd build-buildinged-docker && qmake ../buildinged/tiled.pro INSTALL_ONLY_BUILD=1 && make -j$(nproc) && make install"

sudo chown -R $USER:$USER ../../build-buildinged-docker
