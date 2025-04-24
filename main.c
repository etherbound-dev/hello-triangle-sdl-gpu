#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3_image/SDL_image.h>

static SDL_Window *window = NULL;
static SDL_GPUDevice *device = NULL;
static SDL_GPUGraphicsPipeline *pipeline = NULL;
static SDL_GPUSampler *sampler = NULL;
static SDL_GPUTexture *texture = NULL;
static SDL_GPUBuffer *vertex_buffer = NULL;
static SDL_GPUTexture *depth_texture = NULL;

struct VertexData
{
	float x, y, z; /* 3D data. Vertex range -0.5..0.5 in all axes. Z -0.5 is
			  near, 0.5 is far. */
	float red, green, blue; /* intensity 0 to 1 (alpha is always 1). */
};

static const struct VertexData vertex_data[] = {
	/* Front face. */
	/* Bottom left */
	{-0.5, 0.5, -0.5, 1.0, 0.0, 0.0},  /* red */
	{0.5, -0.5, -0.5, 1.0, 0.0, 0.0},  /* blue */
	{-0.5, -0.5, -0.5, 1.0, 0.0, 0.0}, /* green */

	/* Top right */
	{-0.5, 0.5, -0.5, 1.0, 0.0, 0.0}, /* red */
	{0.5, 0.5, -0.5, 1.0, 0.0, 0.0},  /* yellow */
	{0.5, -0.5, -0.5, 1.0, 0.0, 0.0}, /* blue */

	/* Left face */
	/* Bottom left */
	{-0.5, 0.5, 0.5, 0.0, 1.0, 0.0},   /* white */
	{-0.5, -0.5, -0.5, 0.0, 1.0, 0.0}, /* green */
	{-0.5, -0.5, 0.5, 0.0, 1.0, 0.0},  /* cyan */

	/* Top right */
	{-0.5, 0.5, 0.5, 0.0, 1.0, 0.0},   /* white */
	{-0.5, 0.5, -0.5, 0.0, 1.0, 0.0},  /* red */
	{-0.5, -0.5, -0.5, 0.0, 1.0, 0.0}, /* green */

	/* Top face */
	/* Bottom left */
	{-0.5, 0.5, 0.5, 0.0, 0.0, 1.0},  /* white */
	{0.5, 0.5, -0.5, 0.0, 0.0, 1.0},  /* yellow */
	{-0.5, 0.5, -0.5, 0.0, 0.0, 1.0}, /* red */

	/* Top right */
	{-0.5, 0.5, 0.5, 0.0, 0.0, 1.0}, /* white */
	{0.5, 0.5, 0.5, 0.0, 0.0, 1.0},	 /* black */
	{0.5, 0.5, -0.5, 0.0, 0.0, 1.0}, /* yellow */

	/* Right face */
	/* Bottom left */
	{0.5, 0.5, -0.5, 1.0, 1.0, 0.0},  /* yellow */
	{0.5, -0.5, 0.5, 1.0, 1.0, 0.0},  /* magenta */
	{0.5, -0.5, -0.5, 1.0, 1.0, 0.0}, /* blue */

	/* Top right */
	{0.5, 0.5, -0.5, 1.0, 1.0, 0.0}, /* yellow */
	{0.5, 0.5, 0.5, 1.0, 1.0, 0.0},	 /* black */
	{0.5, -0.5, 0.5, 1.0, 1.0, 0.0}, /* magenta */

	/* Back face */
	/* Bottom left */
	{0.5, 0.5, 0.5, 1.0, 0.0, 1.0},	  /* black */
	{-0.5, -0.5, 0.5, 1.0, 0.0, 1.0}, /* cyan */
	{0.5, -0.5, 0.5, 1.0, 0.0, 1.0},  /* magenta */

	/* Top right */
	{0.5, 0.5, 0.5, 1.0, 0.0, 1.0},	  /* black */
	{-0.5, 0.5, 0.5, 1.0, 0.0, 1.0},  /* white */
	{-0.5, -0.5, 0.5, 1.0, 0.0, 1.0}, /* cyan */

	/* Bottom face */
	/* Bottom left */
	{-0.5, -0.5, -0.5, 0.0, 1.0, 1.0}, /* green */
	{0.5, -0.5, 0.5, 0.0, 1.0, 1.0},   /* magenta */
	{-0.5, -0.5, 0.5, 0.0, 1.0, 1.0},  /* cyan */

	/* Top right */
	{-0.5, -0.5, -0.5, 0.0, 1.0, 1.0}, /* green */
	{0.5, -0.5, -0.5, 0.0, 1.0, 1.0},  /* blue */
	{0.5, -0.5, 0.5, 0.0, 1.0, 1.0}	   /* magenta */
};

static void rotate_matrix(float angle, float x, float y, float z, float *r)
{
	float radians, c, s, c1, u[3], length;
	int i, j;

	radians = angle * SDL_PI_F / 180.0f;

	c = SDL_cosf(radians);
	s = SDL_sinf(radians);

	c1 = 1.0f - SDL_cosf(radians);

	length = (float)SDL_sqrt(x * x + y * y + z * z);

	u[0] = x / length;
	u[1] = y / length;
	u[2] = z / length;

	for (i = 0; i < 16; i++)
	{
		r[i] = 0.0;
	}

	r[15] = 1.0;

	for (i = 0; i < 3; i++)
	{
		r[i * 4 + (i + 1) % 3] = u[(i + 2) % 3] * s;
		r[i * 4 + (i + 2) % 3] = -u[(i + 1) % 3] * s;
	}

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			r[i * 4 + j] += c1 * u[i] * u[j] + (i == j ? c : 0.0f);
		}
	}
}

/*
 * Simulates gluPerspectiveMatrix
 */
static void perspective_matrix(float fovy, float aspect, float znear,
			       float zfar, float *r)
{
	int i;
	float f;

	f = 1.0f / SDL_tanf(fovy * 0.5f);

	for (i = 0; i < 16; i++)
	{
		r[i] = 0.0;
	}

	r[0] = f / aspect;
	r[5] = f;
	r[10] = (znear + zfar) / (znear - zfar);
	r[11] = -1.0f;
	r[14] = (2.0f * znear * zfar) / (znear - zfar);
	r[15] = 0.0f;
}

/*
 * Multiplies lhs by rhs and writes out to r. All matrices are 4x4 and column
 * major. In-place multiplication is supported.
 */
static void multiply_matrix(const float *lhs, const float *rhs, float *r)
{
	int i, j, k;
	float tmp[16];

	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			tmp[j * 4 + i] = 0.0;

			for (k = 0; k < 4; k++)
			{
				tmp[j * 4 + i] +=
					lhs[k * 4 + i] * rhs[j * 4 + k];
			}
		}
	}

	for (i = 0; i < 16; i++)
	{
		r[i] = tmp[i];
	}
}
static SDL_GPUTexture *CreateDepthTexture(Uint32 drawablew, Uint32 drawableh);

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *filename,
			  Uint32 sampler_count, Uint32 uniform_buffer_count,
			  Uint32 storage_buffer_count,
			  Uint32 storage_texture_count);

SDL_Surface *LoadImage(const char *filename);

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	SDL_SetAppMetadata("Hello Triangle", "1.0",
			   "com.example.hello-triangle");

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV |
					     SDL_GPU_SHADERFORMAT_DXIL |
					     SDL_GPU_SHADERFORMAT_MSL,
				     true, NULL);

	if (!device)
	{
		SDL_Log("Couldn't create GPU device: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	window = SDL_CreateWindow("Hello Triangle", 640, 480, 0);
	if (!window)
	{
		SDL_Log("Couldn't create window: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (!SDL_ClaimWindowForGPUDevice(device, window))
	{
		SDL_Log("Couldn't claim window for GPU device: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader *vertex_shader =
		LoadShader(device, "shader.vert", 0, 1, 0, 0);
	if (!vertex_shader)
	{
		SDL_Log("Couldn't load vertex shader: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_GPUShader *fragment_shader =
		LoadShader(device, "shader.frag", 0, 0, 0, 0);
	if (!fragment_shader)
	{
		SDL_Log("Couldn't load fragment shader: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUBufferCreateInfo buffer_desc = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(vertex_data),
	};

	vertex_buffer = SDL_CreateGPUBuffer(device, &buffer_desc);
	if (!vertex_buffer)
	{
		SDL_Log("Couldn't create vertex buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTransferBufferCreateInfo transfer_buffer_desc = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(vertex_data),
	};

	SDL_GPUTransferBuffer *transfer_buffer =
		SDL_CreateGPUTransferBuffer(device, &transfer_buffer_desc);
	if (!transfer_buffer)
	{
		SDL_Log("Couldn't create transfer buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	void *map = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
	SDL_memcpy(map, vertex_data, sizeof(vertex_data));
	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUCommandBuffer *cmd_buf = SDL_AcquireGPUCommandBuffer(device);
	if (!cmd_buf)
	{
		SDL_Log("Couldn't acquire command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(cmd_buf);
	if (!copy_pass)
	{
		SDL_Log("Couldn't begin copy pass: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_GPUTransferBufferLocation transfer_buffer_location = {
		.transfer_buffer = transfer_buffer,
		.offset = 0,
	};
	SDL_GPUBufferRegion vertex_buffer_region = {
		.buffer = vertex_buffer,
		.offset = 0,
		.size = sizeof(vertex_data),
	};
	SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location,
			      &vertex_buffer_region, false);
	SDL_EndGPUCopyPass(copy_pass);
	SDL_SubmitGPUCommandBuffer(cmd_buf);

	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUColorTargetDescription color_target_desc;
	SDL_GPUGraphicsPipelineCreateInfo pipeline_desc;

	SDL_zero(color_target_desc);
	SDL_zero(pipeline_desc);

	color_target_desc.format =
		SDL_GetGPUSwapchainTextureFormat(device, window);

	pipeline_desc.target_info.num_color_targets = 1;
	pipeline_desc.target_info.color_target_descriptions =
		&color_target_desc;
	pipeline_desc.target_info.depth_stencil_format =
		SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	pipeline_desc.target_info.has_depth_stencil_target = true;

	pipeline_desc.depth_stencil_state.enable_depth_test = true;
	pipeline_desc.depth_stencil_state.enable_depth_write = true;
	pipeline_desc.depth_stencil_state.compare_op =
		SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

	pipeline_desc.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	pipeline_desc.vertex_shader = vertex_shader;
	pipeline_desc.fragment_shader = fragment_shader;

	SDL_GPUVertexBufferDescription vertex_buffer_desc;
	SDL_GPUVertexAttribute vertex_attributes[2];

	vertex_buffer_desc.slot = 0;
	vertex_buffer_desc.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
	vertex_buffer_desc.instance_step_rate = 0;
	vertex_buffer_desc.pitch = sizeof(struct VertexData);

	vertex_attributes[0].buffer_slot = 0;
	vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertex_attributes[0].location = 0;
	vertex_attributes[0].offset = 0;

	vertex_attributes[1].buffer_slot = 0;
	vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
	vertex_attributes[1].location = 1;
	vertex_attributes[1].offset = sizeof(float) * 3;

	pipeline_desc.vertex_input_state.num_vertex_buffers = 1;
	pipeline_desc.vertex_input_state.vertex_buffer_descriptions =
		&vertex_buffer_desc;
	pipeline_desc.vertex_input_state.num_vertex_attributes = 2;
	pipeline_desc.vertex_input_state.vertex_attributes =
		(SDL_GPUVertexAttribute *)&vertex_attributes;

	pipeline_desc.props = 0;

	pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_desc);
	if (!pipeline)
	{
		SDL_Log("Failed to create pipeline: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_ReleaseGPUShader(device, vertex_shader);
	SDL_ReleaseGPUShader(device, fragment_shader);

	Uint32 drawablew, drawableh;
	SDL_GetWindowSizeInPixels(window, (int *)&drawablew, (int *)&drawableh);
	depth_texture = CreateDepthTexture(drawablew, drawableh);
	return SDL_APP_CONTINUE;
}
static SDL_GPUTexture *CreateDepthTexture(Uint32 drawablew, Uint32 drawableh)
{
	SDL_GPUTextureCreateInfo createinfo;
	SDL_GPUTexture *result;

	createinfo.type = SDL_GPU_TEXTURETYPE_2D;
	createinfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	createinfo.width = drawablew;
	createinfo.height = drawableh;
	createinfo.layer_count_or_depth = 1;
	createinfo.num_levels = 1;
	createinfo.sample_count = 0;
	createinfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
	createinfo.props = 0;

	result = SDL_CreateGPUTexture(device, &createinfo);
	if (!result)
	{
		SDL_Log("Failed to create depth texture: %s", SDL_GetError());
		return NULL;
	}

	return result;
}
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}
	return SDL_APP_CONTINUE;
}

float angle_x = 0.0f, angle_y = 0.0f, angle_z = 0.0f;
SDL_AppResult SDL_AppIterate(void *appstate)
{
	SDL_GPUCommandBuffer *cmd_buf = SDL_AcquireGPUCommandBuffer(device);
	if (!cmd_buf)
	{
		SDL_Log("Couldn't acquire command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	Uint32 drawablew, drawableh;
	SDL_GPUTexture *swapchain_texture;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, window,
						   &swapchain_texture,
						   &drawablew, &drawableh))
	{
		SDL_Log("Couldn't acquire swapchain texture: %s",
			SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchain_texture == NULL)
	{
		SDL_CancelGPUCommandBuffer(cmd_buf);
		return SDL_APP_CONTINUE;
	}

	float matrix_rotate[16], matrix_modelview[16], matrix_perspective[16],
		matrix_final[16];
	rotate_matrix((float)angle_x, 1.0f, 0.0f, 0.0f, matrix_modelview);
	rotate_matrix((float)angle_y, 0.0f, 1.0f, 0.0f, matrix_rotate);

	multiply_matrix(matrix_rotate, matrix_modelview, matrix_modelview);

	rotate_matrix((float)angle_z, 0.0f, 1.0f, 0.0f, matrix_rotate);

	multiply_matrix(matrix_rotate, matrix_modelview, matrix_modelview);

	/* Pull the camera back from the cube */
	matrix_modelview[14] -= 2.5f;

	perspective_matrix(45.0f, (float)drawablew / drawableh, 0.01f, 100.0f,
			   matrix_perspective);
	multiply_matrix(matrix_perspective, matrix_modelview,
			(float *)&matrix_final);

	angle_x += 0.5f;
	angle_y += 0.5f;
	angle_z += 0.5f;
	if (angle_x >= 360)
		angle_x -= 360;
	if (angle_x < 0)
		angle_x += 360;
	if (angle_y >= 360)
		angle_y -= 360;
	if (angle_y < 0)
		angle_y += 360;
	if (angle_z >= 360)
		angle_z -= 360;
	if (angle_z < 0)
		angle_z += 360;
	SDL_GPUColorTargetInfo color_target_info;
	SDL_zero(color_target_info);
	color_target_info.clear_color.a = 1.0f;
	color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
	color_target_info.store_op = SDL_GPU_STOREOP_STORE;
	color_target_info.texture = swapchain_texture;

	SDL_GPUDepthStencilTargetInfo depth_target_info;
	SDL_zero(depth_target_info);
	depth_target_info.clear_depth = 1.0f;
	depth_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
	depth_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
	depth_target_info.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
	depth_target_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
	depth_target_info.texture = depth_texture;
	depth_target_info.cycle = true;

	SDL_GPUBufferBinding vertex_binding;
	vertex_binding.buffer = vertex_buffer;
	vertex_binding.offset = 0;

	SDL_PushGPUVertexUniformData(cmd_buf, 0, matrix_final,
				     sizeof(matrix_final));
	SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(
		cmd_buf, &color_target_info, 1, &depth_target_info);

	SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
	SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_binding, 1);
	SDL_DrawGPUPrimitives(render_pass, 36, 1, 0, 0);
	SDL_EndGPURenderPass(render_pass);

	SDL_SubmitGPUCommandBuffer(cmd_buf);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *filename,
			  Uint32 sampler_count, Uint32 uniform_buffer_count,
			  Uint32 storage_buffer_count,
			  Uint32 storage_texture_count)
{
	SDL_GPUShaderStage stage;
	if (SDL_strstr(filename, ".vert"))
	{
		stage = SDL_GPU_SHADERSTAGE_VERTEX;
	}
	else if (SDL_strstr(filename, ".frag"))
	{
		stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
	}
	else
	{
		SDL_Log("Unknown shader type: %s", filename);
		return NULL;
	}

	SDL_GPUShaderFormat backend_formats = SDL_GetGPUShaderFormats(device);
	SDL_GPUShaderFormat format = SDL_GPU_SHADERFORMAT_INVALID;
	char fullpath[256];
	const char *entrypoint;
	const char *basepath = SDL_GetBasePath();

	if (backend_formats & SDL_GPU_SHADERFORMAT_SPIRV)
	{
		SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.spv",
			     basepath, filename);
		entrypoint = "main";
		format = SDL_GPU_SHADERFORMAT_SPIRV;
	}
	else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
	{
		SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.dxil",
			     basepath, filename);
		entrypoint = "main";
		format = SDL_GPU_SHADERFORMAT_DXIL;
	}
	else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL)
	{
		SDL_snprintf(fullpath, sizeof(fullpath), "%sshaders/%s.msl",
			     basepath, filename);
		entrypoint = "main0";
		format = SDL_GPU_SHADERFORMAT_MSL;
	}
	else
	{
		SDL_Log("No supported shader format found!");
		return NULL;
	}

	size_t code_size;
	void *code = SDL_LoadFile(fullpath, &code_size);
	if (!code)
	{
		SDL_Log("Couldn't load shader file: %s", SDL_GetError());
		return NULL;
	}

	SDL_GPUShaderCreateInfo shader_info = {
		.code = code,
		.code_size = code_size,
		.entrypoint = entrypoint,
		.format = format,
		.stage = stage,
		.num_samplers = sampler_count,
		.num_uniform_buffers = uniform_buffer_count,
		.num_storage_buffers = storage_buffer_count,
		.num_storage_textures = storage_texture_count,
	};

	SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shader_info);
	if (!shader)
	{
		SDL_Log("Couldn't create shader: %s", SDL_GetError());
		SDL_free(code);
		return NULL;
	}

	SDL_free(code);
	return shader;
}

SDL_Surface *LoadImage(const char *filename)
{

	char fullpath[256];
	const char *basepath = SDL_GetBasePath();
	SDL_Surface *result = NULL;
	SDL_snprintf(fullpath, sizeof(fullpath), "%sresources/%s", basepath,
		     filename);
	SDL_Log("Loading image from %s", fullpath);
	result = IMG_Load(fullpath);
	if (result == NULL)
	{
		SDL_Log("Couldn't load image: %s", SDL_GetError());
		return NULL;
	}

	return result;
}
