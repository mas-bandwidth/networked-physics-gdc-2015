#!/bin/bash

CMFT=cmft

eval $CMFT $@ --input "assets/cubemaps/uffizi-mipmaps.dds"   \
              ::Filter options                  \
              --filter radiance                 \
              --excludeBase false               \
              --glossScale 10                   \
              --glossBias 1                     \
              --lightingModel phongbrdf         \
              --dstFaceSize 512                 \
              ::Processing devices              \
              --numCpuProcessingThreads 4       \
              --useOpenCL true                  \
              --clVendor anyGpuVendor           \
              --deviceType gpu                  \
              --deviceIndex 0                   \
              ::Output                          \
              --output0 "data/cubemaps/uffizi"  \
              --output0params dds,rgba32f,cubemap \

exit

eval $CMFT $@ --input "assets/cubemaps/uffizi.dds"   \
              ::Filter options                  \
              --filter none                     \
              ::Processing devices              \
              --numCpuProcessingThreads 4       \
              --useOpenCL true                  \
              --clVendor anyGpuVendor           \
              --deviceType gpu                  \
              --deviceIndex 1                   \
              --generateMipChain true           \
              ::Output                          \
              --output0 "data/cubemaps/uffizi-mipmaps"  \
              --output0params dds,rgba32f,cubemap \
