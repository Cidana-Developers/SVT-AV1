/*
 * Copyright(c) 2019 Netflix, Inc.
 * SPDX - License - Identifier: BSD - 2 - Clause - Patent
 */

/******************************************************************************
 * @file RefDecoder.h
 *
 * @brief Defines a reference decoder wrapped AOM decoder
 *
 * @author Cidana-Edmond
 *
 ******************************************************************************/
#ifndef _REF_DECODER_H_
#define _REF_DECODER_H_

#include <memory.h>
#include <stdio.h>
#include <string>
#include <vector>
#include "VideoFrame.h"

/** RefDecoder is a class designed for a refenece tool of conformance
 * test. It provides decoding AV1 compressed data with OBU frames, its output is
 * the YUV frame in display order. User should call get_frame right after
 * decode to avoid missing any video frame
 */
class RefDecoder {
  public:
    /** RefDecoderErr is enumerate type of errors from decoder, refered to
     * errors in AOM */
    typedef enum {
        /*!\brief Operation completed without error */
        REF_CODEC_OK,

        /*!\brief Unspecified error */
        REF_CODEC_ERROR = 0 - AOM_CODEC_ERROR,

        /*!\brief Memory operation failed */
        REF_CODEC_MEM_ERROR = 0 - AOM_CODEC_MEM_ERROR,

        /*!\brief ABI version mismatch */
        REF_CODEC_ABI_MISMATCH = 0 - AOM_CODEC_ABI_MISMATCH,

        /*!\brief Algorithm does not have required capability */
        REF_CODEC_INCAPABLE = 0 - AOM_CODEC_INCAPABLE,

        /*!\brief The given bitstream is not supported.
         *
         * The bitstream was unable to be parsed at the highest level. The
         * decoder is unable to proceed. This error \ref SHOULD be treated as
         * fatal to the stream. */
        REF_CODEC_UNSUP_BITSTREAM = 0 - AOM_CODEC_UNSUP_BITSTREAM,

        /*!\brief Encoded bitstream uses an unsupported feature
         *
         * The decoder does not implement a feature required by the encoder.
         * This return code should only be used for features that prevent future
         * pictures from being properly decoded. This error \ref MAY be treated
         * as fatal to the stream or \ref MAY be treated as fatal to the current
         * GOP.
         */
        REF_CODEC_UNSUP_FEATURE = 0 - AOM_CODEC_UNSUP_FEATURE,

        /*!\brief The coded data for this stream is corrupt or incomplete
         *
         * There was a problem decoding the current frame.  This return code
         * should only be used for failures that prevent future pictures from
         * being properly decoded. This error \ref MAY be treated as fatal to
         * the stream or \ref MAY be treated as fatal to the current GOP. If
         * decoding is continued for the current GOP, artifacts may be present.
         */
        REF_CODEC_CORRUPT_FRAME = 0 - AOM_CODEC_CORRUPT_FRAME,

        /*!\brief An application-supplied parameter is not valid.
         *
         */
        REF_CODEC_INVALID_PARAM = 0 - AOM_CODEC_INVALID_PARAM,

        /*!\brief An iterator reached the end of list.
         *
         */
        REF_CODEC_LIST_END = 0 - AOM_CODEC_LIST_END,

        /*!\brief Decoder need more input data to generate frame
         *
         */
        REF_CODEC_NEED_MORE_INPUT = -100,

    } RefDecoderErr;

    typedef struct InspectParams {
        std::vector<int> frame_type_list;
        uint32_t tile_rows;
        uint32_t tile_cols;
        uint32_t min_block_size;
        uint32_t
            ext_block_flag; /**< if contains extended block, 0--no, 1--yes */
        std::vector<uint32_t> qindex_list;
        uint32_t max_qindex;
        uint32_t min_qindex;

        InspectParams() {
            tile_rows = 0;
            tile_cols = 0;
            min_block_size = 128;
            ext_block_flag = 0;
            max_qindex = 0;
            min_qindex = 255;
            frame_type_list.clear();
            qindex_list.clear();
        }
    } InspectParams;

  public:
    /** Constructor of RefDecoder
     * @param ret the error code found in construction
     * @param enable_analyzer the flag to create analyzer with decoder
     */
    RefDecoder(RefDecoderErr &ret, bool enable_analyzer);
    /** Destructor of RefDecoder      */
    virtual ~RefDecoder();

  public:
    /** Decode raw data
     * @param data  the memory buffer of a frame of compressed data
     * @param size  the size of data
     * @return
     * REF_CODEC_OK -- no error found in processing
     * others -- errors found in process, refer to RefDecoderErr
     */
    RefDecoderErr decode(const uint8_t *data, const uint32_t size);

    /** Get a video frame after data proceed
     * @param frame  the video frame with its attributes
     * @return
     * REF_CODEC_OK -- no error found in processing
     * others -- errors found in process, refer to RefDecoderErr
     */
    RefDecoderErr get_frame(VideoFrame &frame);
    /** get parameter value by its name
     * @param name the name of paramter in string
     * @return
     * std::string the value of the paramter in string format
     */
    std::string get_syntax_element(const std::string &name);
    /** get parameter value by its name and index
     * @param name the name of paramter in string
     * @param index the index of paramter value in list
     * @return
     * std::string the value of the paramter in string format
     */
    std::string get_syntax_element(const std::string &name,
                                   const uint32_t index);

    /** Setup resolution, for initialization for inspection frame before first
     * frame
     * @param width  width of source video frame
     * @param height  height of source video frame
     */
    void set_resolution(const uint32_t width, const uint32_t height);

  private:
    /** Tool of translation from AOM image info to a video frame
     * @param image  the video image from AOM decoder
     * @param frame  the video frame to output
     */
    void trans_video_frame(const void *image, VideoFrame &frame);

    /** Callback fuction of inspection frame output */
    static void inspect_cb(void *pbi, void *data);
    /** Tool of pasring inspection frame for its paramters */
    void inspect_frame_parse();

  protected:
    void *codec_handle_;      /**< AOM codec context */
    uint32_t dec_frame_cnt_;  /**< count of decoded frame in processing */
    uint64_t init_timestamp_; /**< initial timestamp of stream */
    uint32_t frame_interval_; /**< time interval of two frame in miliseconds */
    void *insp_frame_data_;   /**< inspect frame data structure */
    VideoFrameParam video_param_; /**< parameter of decoded video frame */
    void *parser_; /**< sequence parser for parameter verification */
    InspectParams insp_params_; /**inspection frame parameters */
    uint32_t enc_bytes_;   /**< total bytes of input, for bit-rate counting */
    uint32_t burst_bytes_; /**< the largest size of input for burst bit-rate */
};

/** Interface of reference decoder creation
 * @param enable_parser  the flag of using inspection frame for parameter check
 * @return
 * RefDecoder -- decoder handle created
 * nullptr -- creation failed
 */
RefDecoder *create_reference_decoder(bool enable_analyzer = false);

#endif  // !_REF_DECODER_H_
