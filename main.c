#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_GPUDevice *device = NULL;
static SDL_GPUGraphicsPipeline *pipeline = NULL;
static SDL_GPUBuffer *vertex_buffer = NULL;

struct PositionColorVertex
{
	float x, y, z;
	Uint8 r, g, b, a;
};

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *filename, Uint32 sampler_count,
			  Uint32 uniform_buffer_count, Uint32 storage_buffer_count, Uint32 storage_texture_count)
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
		SDL_snprintf(fullpath, sizeof(fullpath), "%sresources/%s.spv", basepath, filename);
		entrypoint = "main";
		format = SDL_GPU_SHADERFORMAT_SPIRV;
	}
	else if (backend_formats & SDL_GPU_SHADERFORMAT_DXIL)
	{
		SDL_snprintf(fullpath, sizeof(fullpath), "%sresources/%s.dxil", basepath, filename);
		entrypoint = "main";
		format = SDL_GPU_SHADERFORMAT_DXIL;
	}
	else if (backend_formats & SDL_GPU_SHADERFORMAT_MSL)
	{
		SDL_snprintf(fullpath, sizeof(fullpath), "%sresources/%s.msl", basepath, filename);
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

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
	SDL_SetAppMetadata("Hello Triangle", "1.0", "com.example.hello-triangle");

	if (!SDL_Init(SDL_INIT_VIDEO))
	{
		SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	device = SDL_CreateGPUDevice(
		SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL, false, NULL);

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
		SDL_Log("Couldn't claim window for GPU device: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUShader *vertex_shader = LoadShader(device, "shader.vert", 0, 0, 0, 0);
	if (!vertex_shader)
	{
		SDL_Log("Couldn't load vertex shader: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}
	SDL_GPUShader *fragment_shader = LoadShader(device, "shader.frag", 0, 0, 0, 0);
	if (!fragment_shader)
	{
		SDL_Log("Couldn't load fragment shader: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUGraphicsPipelineCreateInfo pipline_create_info = {
		.target_info =
			{
				.num_color_targets = 1,
				.color_target_descriptions = (SDL_GPUColorTargetDescription[]){{
					.format = SDL_GetGPUSwapchainTextureFormat(device, window),
				}},
			},
		.vertex_input_state =
			(SDL_GPUVertexInputState){
				.num_vertex_buffers = 1,
				.vertex_buffer_descriptions = (SDL_GPUVertexBufferDescription[]){{
					.slot = 0,
					.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
					.instance_step_rate = 0,
					.pitch = sizeof(struct PositionColorVertex),
				}},
				.num_vertex_attributes = 2,
				.vertex_attributes =
					(SDL_GPUVertexAttribute[]){
						{
							.buffer_slot = 0,
							.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
							.location = 0,
							.offset = 0,
						},
						{
							.buffer_slot = 0,
							.format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
							.location = 1,
							.offset = sizeof(float) * 3,
						}},
			},
		.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
		.vertex_shader = vertex_shader,
		.fragment_shader = fragment_shader,
	};

	pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipline_create_info);
	if (!pipeline)
	{
		SDL_Log("Couldn't create graphics pipeline: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_ReleaseGPUShader(device, vertex_shader);
	SDL_ReleaseGPUShader(device, fragment_shader);

	SDL_GPUBufferCreateInfo buffer_create_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = sizeof(struct PositionColorVertex) * 3,
	};
	vertex_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);

	SDL_GPUTransferBufferCreateInfo transfer_buffer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = sizeof(struct PositionColorVertex) * 3,
	};
	SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_info);

	struct PositionColorVertex *transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
	if (!transfer_data)
	{
		SDL_Log("Couldn't map transfer buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	transfer_data[0] = (struct PositionColorVertex){-0.5f, -0.5f, 0.0f, 255, 0, 0, 255};
	transfer_data[1] = (struct PositionColorVertex){0.5f, -0.5f, 0.0f, 0, 255, 0, 255};
	transfer_data[2] = (struct PositionColorVertex){0.0f, 0.5f, 0.0f, 0, 0, 255, 255};

	SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

	SDL_GPUCommandBuffer *upload_cmd_buf = SDL_AcquireGPUCommandBuffer(device);
	if (!upload_cmd_buf)
	{
		SDL_Log("Couldn't acquire command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(upload_cmd_buf);
	if (!copy_pass)
	{
		SDL_Log("Couldn't begin copy pass: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTransferBufferLocation transfer_buf_loc = {
		.transfer_buffer = transfer_buffer,
		.offset = 0,
	};
	SDL_GPUBufferRegion vertex_buf_region = {
		.buffer = vertex_buffer,
		.offset = 0,
		.size = sizeof(struct PositionColorVertex) * 3,
	};

	SDL_UploadToGPUBuffer(copy_pass, &transfer_buf_loc, &vertex_buf_region, false);
	SDL_SubmitGPUCommandBuffer(upload_cmd_buf);
	SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
	if (event->type == SDL_EVENT_QUIT)
	{
		return SDL_APP_SUCCESS;
	}
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
	SDL_GPUCommandBuffer *cmd_buf = SDL_AcquireGPUCommandBuffer(device);
	if (!cmd_buf)
	{
		SDL_Log("Couldn't acquire command buffer: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GPUTexture *swapchain_texture;
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, window, &swapchain_texture, NULL, NULL))
	{
		SDL_Log("Couldn't acquire swapchain texture: %s", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	if (swapchain_texture != NULL)
	{
		SDL_GPUColorTargetInfo color_target_info = { 0 };
		color_target_info.texture = swapchain_texture;
		color_target_info.clear_color = (SDL_FColor){0.0f, 0.0f, 0.0f, 1.0f};
		color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
		color_target_info.store_op = SDL_GPU_STOREOP_STORE;

		SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target_info, 1, NULL);

		SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
		SDL_BindGPUVertexBuffers(render_pass, 0, &(SDL_GPUBufferBinding){ .buffer = vertex_buffer, .offset = 0 }, 1);
		SDL_DrawGPUPrimitives(render_pass, 3, 1, 0, 0);

		SDL_EndGPURenderPass(render_pass);
	}

	SDL_SubmitGPUCommandBuffer(cmd_buf);

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
