#if 1

#include "SDL_opengl.h"
#include "GL/glu.h"

#include "Game.h"

#if 0
static const char* vertex_shader = R"(
varying vec4 v_color;
varying vec2 v_texCoord;
varying vec3 v_position;

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	v_color = gl_Color;
	v_texCoord = gl_MultiTexCoord0.xy;
	v_position = gl_Vertex.xyz;
}
)";

static const char* fragment_shader = R"(
varying vec4 v_color;
varying vec2 v_texCoord;
varying vec3 v_position;

uniform sampler2D tex0;
uniform float camera_x;
uniform float camera_y;
uniform float camera_z;

void main()
{
	a = b;

	vec3 camera_pos = vec3(camera_x, camera_y, camera_z);
	vec3 fog_color = vec3(0.0, 0.0, 0.0);

	vec4 color = texture2D(tex0, v_texCoord);
	float dist = distance(v_position, camera_pos);
	float a = clamp(dist / 400.0, 0.0, 1.0);
	color.rgb = mix(color.rgb, fog_color, a);

	gl_FragColor = color * v_color;
}
)";
#endif

namespace th {

	struct Stage1_Data {
		//PFNGLATTACHOBJECTARBPROC glAttachObjectARB;
		//PFNGLCOMPILESHADERARBPROC glCompileShaderARB;
		//PFNGLCREATEPROGRAMOBJECTARBPROC glCreateProgramObjectARB;
		//PFNGLCREATESHADEROBJECTARBPROC glCreateShaderObjectARB;
		//PFNGLDELETEOBJECTARBPROC glDeleteObjectARB;
		//PFNGLGETINFOLOGARBPROC glGetInfoLogARB;
		//PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB;
		//PFNGLGETUNIFORMLOCATIONARBPROC glGetUniformLocationARB;
		//PFNGLLINKPROGRAMARBPROC glLinkProgramARB;
		//PFNGLSHADERSOURCEARBPROC glShaderSourceARB;
		//PFNGLUNIFORM1IARBPROC glUniform1iARB;
		//PFNGLUNIFORM1FARBPROC glUniform1fARB;
		PFNGLUSEPROGRAMOBJECTARBPROC glUseProgramObjectARB;

		//GLhandleARB shader_program;
		//GLhandleARB shader_vertex_shader;
		//GLhandleARB shader_fragment_shader;
		//
		//GLint shader_u_camera_x;
		//GLint shader_u_camera_y;
		//GLint shader_u_camera_z;

		SDL_Texture* texture;
	};

	void Stage1_Init(Game* ctx, SDL_Renderer* renderer, void* mem) {
		//sizeof(Stage1_Data);

		Stage1_Data* data = (Stage1_Data*)mem;

		data->texture = ctx->assets.GetTexture("MistyLakeTexture.png");
		SDL_SetTextureScaleMode(data->texture, SDL_ScaleModeLinear);

		SDL_RenderFlush(renderer);

		SDL_GL_BindTexture(data->texture, 0, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		SDL_GL_UnbindTexture(data->texture);

		//data->glAttachObjectARB = (PFNGLATTACHOBJECTARBPROC) SDL_GL_GetProcAddress("glAttachObjectARB");
		//data->glCompileShaderARB = (PFNGLCOMPILESHADERARBPROC) SDL_GL_GetProcAddress("glCompileShaderARB");
		//data->glCreateProgramObjectARB = (PFNGLCREATEPROGRAMOBJECTARBPROC) SDL_GL_GetProcAddress("glCreateProgramObjectARB");
		//data->glCreateShaderObjectARB = (PFNGLCREATESHADEROBJECTARBPROC) SDL_GL_GetProcAddress("glCreateShaderObjectARB");
		//data->glDeleteObjectARB = (PFNGLDELETEOBJECTARBPROC) SDL_GL_GetProcAddress("glDeleteObjectARB");
		//data->glGetInfoLogARB = (PFNGLGETINFOLOGARBPROC) SDL_GL_GetProcAddress("glGetInfoLogARB");
		//data->glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC) SDL_GL_GetProcAddress("glGetObjectParameterivARB");
		//data->glGetUniformLocationARB = (PFNGLGETUNIFORMLOCATIONARBPROC) SDL_GL_GetProcAddress("glGetUniformLocationARB");
		//data->glLinkProgramARB = (PFNGLLINKPROGRAMARBPROC) SDL_GL_GetProcAddress("glLinkProgramARB");
		//data->glShaderSourceARB = (PFNGLSHADERSOURCEARBPROC) SDL_GL_GetProcAddress("glShaderSourceARB");
		//data->glUniform1iARB = (PFNGLUNIFORM1IARBPROC) SDL_GL_GetProcAddress("glUniform1iARB");
		//data->glUniform1fARB = (PFNGLUNIFORM1FARBPROC) SDL_GL_GetProcAddress("glUniform1fARB");
		data->glUseProgramObjectARB = (PFNGLUSEPROGRAMOBJECTARBPROC) SDL_GL_GetProcAddress("glUseProgramObjectARB");

#if 0

		data->shader_program = data->glCreateProgramObjectARB();

		data->shader_vertex_shader = data->glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);

		{
			const char* sources[] = {vertex_shader};
			data->glShaderSourceARB(data->shader_vertex_shader, 1, sources, 0);
			data->glCompileShaderARB(data->shader_vertex_shader);
		}

		data->shader_fragment_shader = data->glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

		{
			const char* sources[] = {fragment_shader};
			data->glShaderSourceARB(data->shader_fragment_shader, 1, sources, 0);
			data->glCompileShaderARB(data->shader_fragment_shader);
		}

		data->glAttachObjectARB(data->shader_program, data->shader_vertex_shader);
		data->glAttachObjectARB(data->shader_program, data->shader_fragment_shader);
		data->glLinkProgramARB(data->shader_program);

		data->glUseProgramObjectARB(data->shader_program);

		GLint location = data->glGetUniformLocationARB(data->shader_program, "tex0");
		data->glUniform1iARB(location, 0);

		data->shader_u_camera_x = data->glGetUniformLocationARB(data->shader_program, "camera_x");
		data->shader_u_camera_y = data->glGetUniformLocationARB(data->shader_program, "camera_y");
		data->shader_u_camera_z = data->glGetUniformLocationARB(data->shader_program, "camera_z");

		data->glUseProgramObjectARB(0);

#endif

		SDL_RenderFlush(renderer);
	}

	void Stage1_Quit(Game* ctx, void* mem) {
		Stage1_Data* data = (Stage1_Data*)mem;

		//data->glDeleteObjectARB(data->shader_vertex_shader);
		//data->glDeleteObjectARB(data->shader_fragment_shader);
		//data->glDeleteObjectARB(data->shader_program);
	}

	void Stage1_Update(Game* ctx, void* mem, bool visible, float delta) {
		if (!visible) return;

		Stage1_Data* data = (Stage1_Data*)mem;
	}

	void Stage1_Draw(Game* ctx, SDL_Renderer* renderer, void* mem, bool visible, float delta) {
		if (!visible) return;

		Stage1_Data* data = (Stage1_Data*)mem;

		const unsigned char* key = SDL_GetKeyboardState(0);

		SDL_RenderFlush(renderer);
		{
			glViewport(0, 0, PLAY_AREA_W, PLAY_AREA_H);
			//glViewport(PLAY_AREA_W / 4, PLAY_AREA_H / 4, PLAY_AREA_W / 2, PLAY_AREA_H / 2);

			float r = 2.0f;

			float time = ctx->game_scene->stage->time;
			//float time = 0;

			float camera_x = 128.0f;
			float camera_y = 256.0f * r + 200.0f - fmodf(time / 10.0f, 256.0f);
			float camera_z = 90.0f;

			float target_x = camera_x;
			float target_y = camera_y - 100.0f;
			float target_z = 0.0f;

			if (key[SDL_SCANCODE_SPACE]) camera_z += 100.0f;
			if (key[SDL_SCANCODE_LCTRL]) target_y -= 100.0f;

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluPerspective(-60, (float)PLAY_AREA_W / (float)PLAY_AREA_H, 1, 1000);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			gluLookAt(camera_x, camera_y, camera_z, target_x, target_y, target_z, 0, 0, 1);

			//data->glUseProgramObjectARB(data->shader_program);
			data->glUseProgramObjectARB(0);

			//data->glUniform1fARB(data->shader_u_camera_x, camera_x);
			//data->glUniform1fARB(data->shader_u_camera_y, camera_y);
			//data->glUniform1fARB(data->shader_u_camera_z, camera_z);

			//float fogColor[4] = {0.4f, 0.4f, 0.8f, 1};
			float fogColor[4] = {1, 1, 1, 1};
			glFogfv(GL_FOG_COLOR, fogColor);
			glFogi(GL_FOG_MODE, GL_LINEAR);
			glHint(GL_FOG_HINT, GL_NICEST);
			glFogf(GL_FOG_START, 100);
			glFogf(GL_FOG_END, 400);
			glEnable(GL_FOG);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			SDL_GL_BindTexture(data->texture, 0, 0);
			{
				glBegin(GL_QUADS);
				{
					float off = fmodf(time / 10.0f, 256.0f);

					glColor4f(1, 1, 1, 1);
					glTexCoord2f(-r,     -r    ); glVertex3f( off + 256.0f * -r,       256.0f * -r,       0.0f);
					glTexCoord2f(r+1.0f, -r    ); glVertex3f( off + 256.0f * (r+1.0f), 256.0f * -r,       0.0f);
					glTexCoord2f(r+1.0f, r+1.0f); glVertex3f( off + 256.0f * (r+1.0f), 256.0f * (r+1.0f), 0.0f);
					glTexCoord2f(-r,     r+1.0f); glVertex3f( off + 256.0f * -r,       256.0f * (r+1.0f), 0.0f);

					glColor4f(1, 1, 1, 0.5f);
					glTexCoord2f(-r,     -r    ); glVertex3f(-off + 256.0f * -r,       256.0f * -r,       0.1f);
					glTexCoord2f(r+1.0f, -r    ); glVertex3f(-off + 256.0f * (r+1.0f), 256.0f * -r,       0.1f);
					glTexCoord2f(r+1.0f, r+1.0f); glVertex3f(-off + 256.0f * (r+1.0f), 256.0f * (r+1.0f), 0.1f);
					glTexCoord2f(-r,     r+1.0f); glVertex3f(-off + 256.0f * -r,       256.0f * (r+1.0f), 0.1f);
				}
				glEnd();
			}
			SDL_GL_UnbindTexture(data->texture);

			glDisable(GL_BLEND);

			glDisable(GL_FOG);

			//data->glUseProgramObjectARB(0);

			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D(0, PLAY_AREA_W, PLAY_AREA_H, 0);

			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
		}
		SDL_RenderFlush(renderer);
	}

}

#endif
