#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <chrono>
#include <glm/glm.hpp>

#include <memory>
#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//----------------------
	struct GameObject {
		virtual ~GameObject();

		PlayMode *playmode;

		Scene::Transform *transform;
		std::list< Scene::Drawable >::iterator drawable_idx;

		GameObject(PlayMode *mode, std::string const &mesh_name, glm::vec3 position);

		virtual void start();
		virtual bool update(float elapsed);
		
		virtual void destroy();
	};
	struct Projectile : GameObject {
        glm::vec3 velocity;
	    float lifetime = 2;

		std::shared_ptr<Sound::PlayingSample> sound_loop;

        Projectile(PlayMode *mode, std::string const &mesh_name, glm::vec3 position, glm::vec3 velocity);
        
		virtual bool update(float elapsed) override;

		virtual void destroy() override;
    };
	// struct Player : GameObject {
	// 	virtual bool update(float elapsed) override;
	// 	virtual void destroy() override;
	// }
    std::list<std::unique_ptr<GameObject>> gameobjects;
	//----------------------

	//z-up trackball-style camera controls:
	struct {
		float radius = 7.0f;
		float azimuth = 0.3f; //angle ccw of -y axis, in radians, [-pi,pi]
		float elevation = 0.2f; //angle above ground, in radians, [-pi,pi]
		glm::vec3 target = glm::vec3(0.0f);
		bool flip_x = false; //flip x inputs when moving? (used to handle situations where camera is upside-down)
	} camera_controls;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;
	bool game_over = false;

	Scene::Transform *player;
	float spawn_timer = 1;

	float score = 0;


	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > leg_tip_loop;

	//car honk sound:
	std::shared_ptr< Sound::PlayingSample > honk_oneshot;
	
	//camera:
	Scene::Camera *camera = nullptr;
};
