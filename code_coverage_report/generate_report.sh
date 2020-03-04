#!/bin/bash
# check if we can find ../code_coverage_report
base_path=`pwd`
SOURCE_DIR=${base_path}/../
BUILD_DIR=${base_path}/build
#INFO_OUTPUT_DIR=${base_path}/info_output
OUTPUT_DIR=${base_path}/output
tmp=../
tmp2=../../C

if [ ! -d "$SOURCE_DIR" ]; then
    exit 1
else
    echo "find source dir"
fi

echo "SOURCE_DIR: $SOURCE_DIR"
echo "BUILD_DIR: $BUILD_DIR"
echo "INFO_OUTPUT_DIR: $INFO_OUTPUT_DIR"
echo "OUTPUT_DIR: $OUTPUT_DIR"
echo "tmp: $tmp"
echo "tmp2: $tmp2"

#exit 0
#read -p "Press any key to continue." var

mkdir $BUILD_DIR
#mkdir $INFO_OUTPUT_DIR
mkdir $OUTPUT_DIR
cd $BUILD_DIR
# config options
cmake "$SOURCE_DIR" -DCMAKE_C_COMPILER=/usr/bin/gcc -DCMAKE_CXX_COMPILER=/usr/bin/g++ -DCMAKE_C_OUTPUT_EXTENSION_REPLACE=1 -DCMAKE_CXX_OUTPUT_EXTENSION_REPLACE=1 -DCMAKE_BUILD_TYPE=Debug -DCOVERAGE=ON -DBUILD_TESTING=ON -DBUILD_SHARED_LIBS=ON
# build
make -j8 SvtAv1EncApp SvtAv1DecApp SvtAv1UnitTests SvtAv1ApiTests SvtAv1E2ETests TestVectors
# Initial
lcov --capture --initial --base-directory $SOURCE_DIR  --directory . --output-file svt_av1_base.info

# run the unit tests
cp "$SOURCE_DIR"Bin/Debug/* .
# copy conformance tools
if [ "$(uname -s)" = "Darwin" ]; then
    cp "$SOURCE_DIR"third_party/aom/lib/mac/*.dylib ./
else
    cp "$SOURCE_DIR"third_party/aom/lib/linux/libaom.so ./libaom.so.0
fi

export GTEST_TOTAL_SHARDS=10
seq 0 $(( $GTEST_TOTAL_SHARDS - 1 )) \
       | xargs -n 1 -P 10 -I{} env GTEST_SHARD_INDEX={} ./SvtAv1UnitTests
unset GTEST_TOTAL_SHARDS
./SvtAv1E2ETests --gtest_filter="SvtAv1/ConformanceDeathTest.DefaultSettingTest/EncModeTest*"
./SvtAv1E2ETests --gtest_filter="SvtAv1/ConformanceDeathTest.DefaultSettingTest/ScreenToolTest2*"
./SvtAv1E2ETests --gtest_filter="SvtAv1/ConformanceDeathTest.DefaultSettingTest/PaletteModeTest*"

# test all cases
# following cases take much time for long time full test
if [ "$1" = "all" ]; then
./SvtAv1ApiTests --gtest_filter=EncApiTest.repeat_normal_setup
fi

# find all ivf files to decoder
for file_list in $(ls .)
do
    if [ ${file_list##*.} = "ivf" ]; then
        echo "decode ${file_list}"
        # setup for output YUV file
        output_yuv=${file_list%.*}.yuv
        ./SvtAv1DecApp -i ${file_list} -o ${output_yuv}
        # delete the output YUV file to save disk space
        rm -f ${output_yuv}
    fi
done

# capture
lcov --capture --base-directory $SOURCE_DIR --directory . --output-file svt_av1_test.info

# merge the status
lcov --add-tracefile svt_av1_base.info --add-tracefile svt_av1_test.info --output-file $BUILD_DIR/svt_av1_total.info

# remove unwanted
lcov -r svt_av1_total.info "*third_party*" "*test*" "*/usr/*" "*App*" -o svt_av1_final.info

genhtml svt_av1_final.info --output-directory $OUTPUT_DIR
