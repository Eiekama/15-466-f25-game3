#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "Sound.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <chrono>
#include <glm/gtc/type_ptr.hpp>

#include <list>
#include <memory>
#include <random>

GLuint hexapod_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > hexapod_meshes(LoadTagDefault, []() -> MeshBuffer const * {
	MeshBuffer const *ret = new MeshBuffer(data_path("game.pnct"));
	hexapod_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > hexapod_scene(LoadTagDefault, []() -> Scene const * {
	return new Scene(data_path("game.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name){
		Mesh const &mesh = hexapod_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > projectile_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("projectile.wav"));
});

Load< Sound::Sample > bgm_sample(LoadTagDefault, []() -> Sound::Sample const * {
	return new Sound::Sample(data_path("bgm.wav"));
});


bool random_bool() {
	return rand() % 2;
}

// from https://stackoverflow.com/questions/686353/random-float-number-generation
float random_float(float low, float high) {
	return low + static_cast <float> (rand()) / (static_cast <float> (RAND_MAX/(high-low)));
}

//----------------------

PlayMode::GameObject::~GameObject() {}

PlayMode::GameObject::GameObject(PlayMode *mode, std::string const &mesh_name, glm::vec3 position) {
	playmode = mode;
	Mesh const &mesh = hexapod_meshes->lookup(mesh_name);
    transform = new Scene::Transform();
    transform->name = mesh_name;
    transform->position = position;
    playmode->scene.drawables.emplace_back(transform);
    Scene::Drawable &drawable = playmode->scene.drawables.back();

    drawable.pipeline = lit_color_texture_program_pipeline;

    drawable.pipeline.vao = hexapod_meshes_for_lit_color_texture_program;
    drawable.pipeline.type = mesh.type;
    drawable.pipeline.start = mesh.start;
    drawable.pipeline.count = mesh.count;

    drawable_idx = std::prev(playmode->scene.drawables.end());
}


void PlayMode::GameObject::start() {}
bool PlayMode::GameObject::update(float elapsed) { return false; }
	
void PlayMode::GameObject::destroy() {
    playmode->scene.drawables.erase(drawable_idx);
	delete transform;
}

PlayMode::Projectile::Projectile(PlayMode *mode, std::string const &mesh_name, glm::vec3 position, glm::vec3 velocity) :
    GameObject(mode, mesh_name, position), velocity(velocity) {
	sound_loop = Sound::loop_3D(*projectile_sample, 1.5f, position, 3.0f);
}

bool PlayMode::Projectile::update(float elapsed) {
    transform->position += velocity * elapsed;
	sound_loop->set_position(transform->position, 1.0f / 60.0f);
    lifetime -= elapsed;
    if (lifetime < 0) {
        destroy();
		return true;
    }
	return false;
}

void PlayMode::Projectile::destroy() {
	sound_loop->stop();
	GameObject::destroy();
}
//----------------------
// PlayMode header implementation below

PlayMode::PlayMode() : scene(*hexapod_scene) {
	for (auto &transform : scene.transforms) {
		if (transform.name == "Player") {
			player = &transform;
			break;
		}
	}

	camera_controls.target = player->position + glm::vec3(0, 0, 2);
	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	Sound::loop(*bgm_sample, 0.5f);
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {

	if (evt.type == SDL_EVENT_KEY_DOWN) {
		if (evt.key.key == SDLK_ESCAPE) {
			SDL_SetWindowRelativeMouseMode(Mode::window, false);
			return true;
		} else if (evt.key.key == SDLK_R) {
			if (game_over) {
				Mode::set_current(std::make_shared< PlayMode >());
			}
		} else if (evt.key.key == SDLK_A) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.downs += 1;
			down.pressed = true;
			return true;
		}
	} else if (evt.type == SDL_EVENT_KEY_UP) {
		if (evt.key.key == SDLK_A) {
			left.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_D) {
			right.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_W) {
			up.pressed = false;
			return true;
		} else if (evt.key.key == SDLK_S) {
			down.pressed = false;
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == false) {
			SDL_SetWindowRelativeMouseMode(Mode::window, true);
			return true;
		}
	} else if (evt.type == SDL_EVENT_MOUSE_MOTION) {
		if (SDL_GetWindowRelativeMouseMode(Mode::window) == true && !game_over) {
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y)
			);

			camera_controls.azimuth -= 3.0f * motion.x * (camera_controls.flip_x ? -1.0f : 1.0f);
			camera_controls.elevation -= 3.0f * motion.y;

			camera_controls.azimuth /= 2.0f * 3.1415926f;
			camera_controls.azimuth -= std::round(camera_controls.azimuth);
			camera_controls.azimuth *= 2.0f * 3.1415926f;

			camera_controls.elevation /= 2.0f * 3.1415926f;
			camera_controls.elevation -= std::round(camera_controls.elevation);
			camera_controls.elevation *= 2.0f * 3.1415926f;

			// // credits to Matteo (dragonsandpizzas) on discord for finding the solution to unintentional roll on
			// // https://stackoverflow.com/questions/46738139/prevent-rotation-around-certain-axis-with-quaternion
			// camera->transform->rotation = glm::normalize(
			// 	glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 0.0f, 1.0f))
			// 	* camera->transform->rotation
			// 	* glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f))
			// );
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (!game_over) {
		score += elapsed;

		if (spawn_timer < 0) {
			float p = random_float(0, 1);
			float offset = random_float(-15, 15);
			if (p < 0.25) {
				gameobjects.emplace_back(std::make_unique<PlayMode::Projectile>(
					this, "Obstacle", glm::vec3(offset,-15,2), glm::vec3(0,20,0)
				));
			}
			else if (p < 0.5) {
				gameobjects.emplace_back(std::make_unique<PlayMode::Projectile>(
					this, "Obstacle", glm::vec3(offset,15,2), glm::vec3(0,-20,0)
				));
			}
			else if (p < 0.75) {
				gameobjects.emplace_back(std::make_unique<PlayMode::Projectile>(
					this, "Obstacle", glm::vec3(-15,offset,2), glm::vec3(20,0,0)
				));
			}
			else {
				gameobjects.emplace_back(std::make_unique<PlayMode::Projectile>(
					this, "Obstacle", glm::vec3(15,offset,2), glm::vec3(-20,0,0)
				));
			}
			spawn_timer = 0.5;
		}
		else {
			spawn_timer -= elapsed;
		}


		std::list<std::unique_ptr<GameObject>>::iterator go;
		for (go = gameobjects.begin(); go != gameobjects.end();) {

			if (glm::length(player->position - (*go)->transform->position) < 1.5) {
				Sound::stop_all_samples();
				game_over = true;
			}

			if ((*go)->update(elapsed)) {
				go = gameobjects.erase(go);
			} else {
				++go;
			}
		}

		//move camera:
		{
			//combine inputs into a move:
			constexpr float PlayerSpeed = 30.0f;
			glm::vec2 move = glm::vec2(0.0f);
			if (left.pressed && !right.pressed) move.x =-1.0f;
			if (!left.pressed && right.pressed) move.x = 1.0f;
			if (down.pressed && !up.pressed) move.y =-1.0f;
			if (!down.pressed && up.pressed) move.y = 1.0f;

			//make it so that moving diagonally doesn't go faster:
			if (move != glm::vec2(0.0f)) {
				move = glm::normalize(move) * PlayerSpeed * elapsed;

				glm::mat4x3 frame = camera->transform->make_parent_from_local();
				glm::vec3 frame_right = frame[0];
				//glm::vec3 up = frame[1];
				glm::vec3 frame_forward = -frame[2];

				glm::vec3 player_right = glm::normalize(glm::vec3(frame_right.x, frame_right.y, 0));
				glm::vec3 player_forward = glm::normalize(glm::vec3(frame_forward.x, frame_forward.y, 0));

				glm::vec3 player_move = move.x * player_right + move.y * player_forward;

				// written with reference to Matteo's code at https://github.com/matteoJeulin/15-466-game2/blob/main/GameMode.cpp
				glm::vec3 up_axis = glm::vec3(0,0,1);
				glm::vec3 forward_axis = glm::vec3(1,0,0);
				float dot = glm::dot(forward_axis, player_move);
				float phi = glm::acos(dot);
				glm::vec3 alignementVector = glm::cross(up_axis, forward_axis);

				// Check if we need to turn left or right
				if (glm::dot(alignementVector, player_move) < 0)
				{
					phi = -phi;
				}

				player->rotation = glm::angleAxis(phi, up_axis);
				// std::cout << player->rotation.x << ' ' << player->rotation.y << ' ' << player->rotation.z << std::endl;


				player->position += player_move;
				if (glm::length(glm::vec2(player->position.x, player->position.y)) > 20) {
					player->position -= player_move;
				}
				else {
					camera_controls.target += player_move;
				}
			}
		}

		{ //update listener to camera position:
			glm::mat4x3 frame = camera->transform->make_parent_from_local();
			glm::vec3 frame_right = frame[0];
			// glm::vec3 frame_at = frame[3];
			Sound::listener.set_position_right(player->position, frame_right, 1.0f / 60.0f);
		}
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//update camera aspect ratio for drawable:
	camera->transform->rotation =
		glm::angleAxis(camera_controls.azimuth, glm::vec3(0.0f, 0.0f, 1.0f))
		* glm::angleAxis(0.5f * 3.1415926f + -camera_controls.elevation, glm::vec3(1.0f, 0.0f, 0.0f))
	;
	camera->transform->position = camera_controls.target + camera_controls.radius * (camera->transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f));
	camera->transform->scale = glm::vec3(1.0f);
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 1);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, 0.0f,-1.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(glm::vec3(1.0f, 1.0f, 0.95f)));
	glUseProgram(0);

	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text("Mouse motion rotates camera; WASD moves; escape ungrabs mouse",
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + + 0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		if (game_over) {
			lines.draw_text(std::format("Game over! Survived for {}s", std::round(score)),
			glm::vec3(-aspect + 0.1f * H, -1.0 + 1.6f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		}
	}
	GL_ERRORS();
}