#!/bin/sh

# run in project root directory

remote_model="https://github.com/spotify/basic-pitch/raw/main/basic_pitch/saved_models/icassp_2022/nmp.onnx"

hash=$(curl -sL $remote_model | sha256sum | cut -d' ' -f1)

if [ "$hash" = "2c3c1d144bfa61ad236e92e169c13535c880469a12a047d4e73451f2c059a0ec" ]; then
    wget $remote_model -O build/basic_pitch.onnx
else
    printf "remote model hash does not match; did not fetch model\n$hash\n"
fi

