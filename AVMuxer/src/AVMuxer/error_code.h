#pragma once

enum Error_ID {
	E_NO_ERROR = 0,
	E_ERROR,
	E_UNSUPPORT,
	E_INVALID_CONTEXT,
	E_NEED_INIT,
	E_TIMEOUT,
	E_ALLOCATE_FAILED,

	// E_CO_
	E_CO_INITED_FAILED,
	E_CO_CREATE_FAILED,
	E_CO_GETENDPOINT_FAILED,
	E_CO_ACTIVE_DEVICE_FAILED,
	E_CO_GET_FORMAT_FAILED,
	E_CO_AUDIOCLIENT_INIT_FAILED,
	E_CO_GET_CAPTURE_FAILED,
	E_CO_CREATE_EVENT_FAILED,
	E_CO_SET_EVENT_FAILED,
	E_CO_START_FAILED,
	E_CO_ENUMENDPOINT_FAILED,
	E_CO_GET_ENDPOINT_COUNT_FAILED,
	E_CO_GET_ENDPOINT_ID_FAILED,
	E_CO_OPEN_PROPERTY_FAILED,
	E_CO_GET_VALUE_FAILED,
	E_CO_GET_BUFFER_FAILED,
	E_CO_RELEASE_BUFFER_FAILED,
	E_CO_GET_PACKET_FAILED,
	E_CO_PADDING_UNEXPECTED,

	// E_FFMPEG_
	E_FFMPEG_OPEN_INPUT_FAILED,
	E_FFMPEG_FIND_STREAM_FAILED,
	E_FFMPEG_FIND_DECODER_FAILED,
	E_FFMPEG_OPEN_CODEC_FAILED,
	E_FFMPEG_READ_FRAME_FAILED,
	E_FFMPEG_READ_PACKET_FAILED,
	E_FFMPEG_DECODE_FRAME_FAILED,
	E_FFMPEG_NEW_SWSCALE_FAILED,
	E_FFMPEG_FIND_ENCODER_FAILED,
	E_FFMPEG_ALLOC_CONTEXT_FAILED,
	E_FFMPEG_ENCODE_FRAME_FAILED,
	E_FFMPEG_ALLOC_FRAME_FAILED,
	E_FFMPEG_OPEN_IO_FAILED,
	E_FFMPEG_CREATE_STREAM_FAILED,
	E_FFMPEG_COPY_PARAMS_FAILED,
	E_RESAMPLE_INIT_FAILED,
	E_FFMPEG_NEW_STREAM_FAILED,
	E_FFMPEG_FIND_INPUT_FMT_FAILED,
	E_FFMPEG_WRITE_HEADER_FAILED,
	E_FFMPEG_WRITE_TRAILER_FAILED,
	E_FFMPEG_WRITE_FRAME_FAILED,

	// E_FILTER_
	E_FILTER_ALLOC_GRAPH_FAILED,
	E_FILTER_CREATE_FILTER_FAILED,
	E_FILTER_PARSE_PTR_FAILED,
	E_FILTER_CONFIG_FAILED,
	E_FILTER_INVALID_CTX_INDEX,
	E_FILTER_ADD_FRAME_FAILED,

	// E_GDI_
	E_GDI_GET_DC_FAILED,
	E_GDI_CREATE_DC_FAILED,
	E_GDI_CREATE_BMP_FAILED,
	E_GDI_BITBLT_FAILED,
	E_GDI_GET_DIBITS_FAILED,

	// E_D3D_
	E_D3D_LOAD_FAILED,
	E_D3D_GET_PROC_FAILED,
	E_D3D_CREATE_DEVICE_FAILED,
	E_D3D_QUERYINTERFACE_FAILED,
	E_D3D_CREATE_VERTEX_SHADER_FAILED,
	E_D3D_CREATE_INLAYOUT_FAILED,
	E_D3D_CREATE_PIXEL_SHADER_FAILED,
	E_D3D_CREATE_SAMPLERSTATE_FAILED,

	// E_DXGI_
	E_DXGI_GET_PROC_FAILED,
	E_DXGI_GET_ADAPTER_FAILED,
	E_DXGI_GET_FACTORY_FAILED,
	E_DXGI_FOUND_ADAPTER_FAILED,

	// E_DUP_
	E_DUP_ATTATCH_FAILED,
	E_DUP_QI_FAILED,
	E_DUP_GET_PARENT_FAILED,
	E_DUP_ENUM_OUTPUT_FAILED,
	E_DUP_DUPLICATE_MAX_FAILED,
	E_DUP_DUPLICATE_FAILED,
	E_DUP_RELEASE_FRAME_FAILED,
	E_DUP_ACQUIRE_FRAME_FAILED,
	E_DUP_QI_FRAME_FAILED,
	E_DUP_CREATE_TEXTURE_FAILED,
	E_DUP_QI_DXGI_FAILED,
	E_DUP_MAP_FAILED,
	E_DUP_GET_CURSORSHAPE_FAILED,

	// E_REMUX_
	E_REMUX_RUNNING,
	E_REMUX_NOT_EXIST,
	E_REMUX_INVALID_INOUT,

	E_MAXIMUM
};

static const char * ERROR_STRS[] = {
	"no error",                             // E_NO_ERROR
	"error",                                // E_ERROR
	"not support for now",                  // E_UNSUPPORT
	"invalid context",                      // E_INVALID_CONTEXT
	"need init first",                      // E_NEED_INIT
	"operation timeout",                    // E_TIMEOUT
	"allocate memory failed",               // E_ALLOCATE_FAILED,

	"com init failed",                      // E_CO_INITED_FAILED
	"com create instance failed",           // E_CO_CREATE_FAILED
	"com get endpoint failed",              // E_CO_GETENDPOINT_FAILED
	"com active device failed",             // E_CO_ACTIVE_DEVICE_FAILED
	"com get wave formatex failed",         // E_CO_GET_FORMAT_FAILED
	"com audio client init failed",         // E_CO_AUDIOCLIENT_INIT_FAILED
	"com audio get capture failed",         // E_CO_GET_CAPTURE_FAILED
	"com audio create event failed",        // E_CO_CREATE_EVENT_FAILED
	"com set ready event failed",           // E_CO_SET_EVENT_FAILED
	"com start to record failed",           // E_CO_START_FAILED
	"com enum audio endpoints failed",      // E_CO_ENUMENDPOINT_FAILED
	"com get endpoints count failed",       // E_CO_GET_ENDPOINT_COUNT_FAILED
	"com get endpoint id failed",           // E_CO_GET_ENDPOINT_ID_FAILED
	"com open endpoint property failed",    // E_CO_OPEN_PROPERTY_FAILED
	"com get property value failed",        // E_CO_GET_VALUE_FAILED
	"com get buffer failed",                // E_CO_GET_BUFFER_FAILED
	"com release buffer failed",            // E_CO_RELEASE_BUFFER_FAILED
	"com get packet size failed",           // E_CO_GET_PACKET_FAILED
	"com get padding size unexpected",      // E_CO_PADDING_UNEXPECTED

	"ffmpeg open input failed",             // E_FFMPEG_OPEN_INPUT_FAILED
	"ffmpeg find stream info failed",       // E_FFMPEG_FIND_STREAM_FAILED
	"ffmpeg find decoder failed",           // E_FFMPEG_FIND_DECODER_FAILED
	"ffmpeg open codec failed",             // E_FFMPEG_OPEN_CODEC_FAILED
	"ffmpeg read frame failed",             // E_FFMPEG_READ_FRAME_FAILED
	"ffmpeg read packet failed",            // E_FFMPEG_READ_PACKET_FAILED
	"ffmpeg decode frame failed",           // E_FFMPEG_DECODE_FRAME_FAILED
	"ffmpeg create swscale failed",         // E_FFMPEG_NEW_SWSCALE_FAILED

	"ffmpeg find encoder failed",           // E_FFMPEG_FIND_ENCODER_FAILED
	"ffmpeg alloc context failed",          // E_FFMPEG_ALLOC_CONTEXT_FAILED
	"ffmpeg encode frame failed",           // E_FFMPEG_ENCODE_FRAME_FAILED
	"ffmpeg alloc frame failed",            // E_FFMPEG_ALLOC_FRAME_FAILED
	
	"ffmpeg open io ctx failed",            // E_FFMPEG_OPEN_IO_FAILED
	"ffmpeg new stream failed",             // E_FFMPEG_CREATE_STREAM_FAILED
	"ffmpeg copy parameters failed",        // E_FFMPEG_COPY_PARAMS_FAILED
	"resampler init failed",                // E_RESAMPLE_INIT_FAILED
	"ffmpeg new out stream failed",         // E_FFMPEG_NEW_STREAM_FAILED
	"ffmpeg find input format failed",      // E_FFMPEG_FIND_INPUT_FMT_FAILED
	"ffmpeg write file header failed",      // E_FFMPEG_WRITE_HEADER_FAILED
	"ffmpeg write file trailer failed",     // E_FFMPEG_WRITE_TRAILER_FAILED
	"ffmpeg write frame failed",            // E_FFMPEG_WRITE_FRAME_FAILED

	"avfilter alloc avfilter failed",       // E_FILTER_ALLOC_GRAPH_FAILED
	"avfilter create graph failed",         // E_FILTER_CREATE_FILTER_FAILED
	"avfilter parse ptr failed",            // E_FILTER_PARSE_PTR_FAILED
	"avfilter config graph failed",         // E_FILTER_CONFIG_FAILED
	"avfilter invalid ctx index",           // E_FILTER_INVALID_CTX_INDEX
	"avfilter add frame failed",            // E_FILTER_ADD_FRAME_FAILED

	"gdi get dc failed",                    // E_GDI_GET_DC_FAILED
	"gdi create dc failed",                 // E_GDI_CREATE_DC_FAILED
	"gdi create bmp failed",                // E_GDI_CREATE_BMP_FAILED
	"gdi bitblt failed",                    // E_GDI_BITBLT_FAILED
	"gid geet dibbits failed",              // E_GDI_GET_DIBITS_FAILED

	"d3d11 library load failed",            // E_D3D_LOAD_FAILED
	"d3d11 proc get failed",                // E_D3D_GET_PROC_FAILED
	"d3d11 create device failed",           // E_D3D_CREATE_DEVICE_FAILED
	"d3d11 query interface failed",         // E_D3D_QUERYINTERFACE_FAILED
	"d3d11 create vertex shader failed",    // E_D3D_CREATE_VERTEX_SHADER_FAILED
	"d3d11 create input layout failed",     // E_D3D_CREATE_INLAYOUT_FAILED
	"d3d11 create pixel shader failed",     // E_D3D_CREATE_PIXEL_SHADER_FAILED
	"d3d11 create sampler state failed",    // E_D3D_CREATE_SAMPLERSTATE_FAILED

	"dxgi get proc address failed",         // E_DXGI_GET_PROC_FAILED
	"dxgi get adapter failed",              // E_DXGI_GET_ADAPTER_FAILED
	"dxgi get factory failed",              // E_DXGI_GET_FACTORY_FAILED
	"dxgi specified adapter not found",     // E_DXGI_FOUND_ADAPTER_FAILED

	"duplication attatch desktop failed",   // E_DUP_ATTATCH_FAILED
	"duplication query interface failed",   // E_DUP_QI_FAILED
	"duplication get parent failed",        // E_DUP_GET_PARENT_FAILED
	"duplication enum ouput failed",        // E_DUP_ENUM_OUTPUT_FAILED
	"duplication duplicate unavailable",    // E_DUP_DUPLICATE_MAX_FAILED
	"duplication duplicate failed",         // E_DUP_DUPLICATE_FAILED
	"duplication release frame failed",     // E_DUP_RELEASE_FRAME_FAILED
	"duplication acquire frame failed",     // E_DUP_ACQUIRE_FRAME_FAILED
	"duplication qi frame failed",          // E_DUP_QI_FRAME_FAILED
	"duplication create texture failed",    // E_DUP_CREATE_TEXTURE_FAILED
	"duplication dxgi qi failed",           // E_DUP_QI_DXGI_FAILED
	"duplication map rects failed",         // E_DUP_MAP_FAILED
	"duplication get cursor shape failed",  // E_DUP_GET_CURSORSHAPE_FAILED

	"remux is already running",             // E_REMUX_RUNNING
	"remux input file do not exist",        // E_REMUX_NOT_EXIST
	"remux input or output file invalid",   // E_REMUX_INVALID_INOUT
};

#define ErrCode(err)        (-(err))
#define Err2Str(err)        (((err) < E_MAXIMUM) ? ERROR_STRS[err] : "unknown")

#define ERROR_CHECK(err)    if (err != E_NO_ERROR) return ErrCode(err)
