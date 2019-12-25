#include "ParticleSystem.hpp"

ParticleSystem::ParticleSystem(const Config& config) :
	_window(config.window.width, config.window.height, "OpenGL_4.3_Particle_System", config.window.fullscreen),
	_input(),
	_camera(config.camera.speed, config.camera.sensitivity, config.camera.foV, config.window.width, config.window.height, config.camera.nearDist, config.camera.farDist),
	_attractor(),
	_particleBuffer(config.particles.numParticles, config.particles.initRadius),
	_particleTexture(),
	_quadLength(config.particles.sizeOfParticles),
	_showFPS(false),
	_computeProgID(0), _shaderProgID(0), _ppShaderProgID(0),
	_deltaTime(0),
	_tmpDeltaTime(0),
	_shaderManager()
  {}

ParticleSystem::~ParticleSystem(){
  deleteParticleSystem();
}

void ParticleSystem::initialize(){
  //////// Initialize GLFW window, input, rendering context and gl3w
  _window.initialize();
  _window.setVSync(true);
  _input.bindInputToWindow(_window);
  
  if(gl3wInit()) throw std::runtime_error("Could not initialize gl3w!");
  if(!gl3wIsSupported(4, 3)) throw std::runtime_error("OpenGL 4.3 not supported!");
  
  std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
  std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
  
  // Generate Vertex Arrays and initialize particles
  _particleBuffer.initializeParticles();
  
  glGenVertexArrays(1, &_vertexArrayID);
  glBindVertexArray(_vertexArrayID);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _particleBuffer.getParticleBufferID());
  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glEnableVertexAttribArray(2);
  glBindBuffer(GL_ARRAY_BUFFER, _particleBuffer.getParticleBufferID());
  glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (GLvoid*)0);
  glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), (GLvoid*)(18 * sizeof(float)));
  glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Particle), (GLvoid*)(20 * sizeof(float)));
  glBindVertexArray(0);

  // Initialize post-processing buffer
  glGenVertexArrays(1, &_ppVertexArrayID);
  glBindVertexArray(_ppVertexArrayID);

  float verts[5 * 6] = {
  -1.0f, -1.0f, 0.0f,  0.0f,  0.0f,
   1.0f, -1.0f, 0.0f,  1.0f,  0.0f,
  -1.0f,  1.0f, 0.0f,  0.0f,  1.0f,
   1.0f,  1.0f, 0.0f,  1.0f,  1.0f,
  -1.0f,  1.0f, 0.0f,  0.0f,  1.0f,
   1.0f, -1.0f, 0.0f,  1.0f,  0.0f
  };

  glGenBuffers(1, &_ppVertexBufferID);
  glBindBuffer(GL_ARRAY_BUFFER, _ppVertexBufferID);
  glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)0);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(3 * sizeof(float)));
  glBindVertexArray(0);

  // Initiliaze shader and shader program
  _shaderManager.loadShader("vs.glsl", "vertexShader", GL_VERTEX_SHADER);
  _shaderManager.loadShader("gs.glsl", "geometryShader", GL_GEOMETRY_SHADER);
  _shaderManager.loadShader("fs.glsl", "fragmentShader", GL_FRAGMENT_SHADER);
  _shaderManager.loadShader("cs.glsl", "computeShader", GL_COMPUTE_SHADER);
  _shaderManager.loadShader("ppvs.glsl", "ppVertexShader", GL_VERTEX_SHADER);
  _shaderManager.loadShader("ppfs.glsl", "ppFragmentShader", GL_FRAGMENT_SHADER);

  _shaderProgID = _shaderManager.createProgram("shaderProg");
  _ppShaderProgID = _shaderManager.createProgram("ppShaderProg");
  _computeProgID = _shaderManager.createProgram("computeProg");

  _shaderManager.attachShader("vertexShader", "shaderProg");
  _shaderManager.attachShader("geometryShader", "shaderProg");
  _shaderManager.attachShader("fragmentShader", "shaderProg");
  _shaderManager.attachShader("ppVertexShader", "ppShaderProg");
  _shaderManager.attachShader("ppFragmentShader", "ppShaderProg");
  _shaderManager.attachShader("computeShader", "computeProg");

  _shaderManager.linkProgram("computeProg");
  _shaderManager.linkProgram("shaderProg");
  _shaderManager.linkProgram("ppShaderProg");

  // Since the programs are linked, the shaders are not needed anymore
  _shaderManager.deleteShader("vertexShader");
  _shaderManager.deleteShader("geometryShader");
  _shaderManager.deleteShader("fragmentShader");
  _shaderManager.deleteShader("ppVertexShader");
  _shaderManager.deleteShader("ppFragmentShader");
  _shaderManager.deleteShader("computeShader");

  _particleTexture.loadTexture("Particle.tga");
  
  // Retrieve the uniform locations for both shaders and set the 
  // uniform variables which will not change for every frame.
  _shaderManager.useProgram(_computeProgID);
  _shaderManager.loadUniform_(_computeProgID, "maxParticles", _particleBuffer.getNumParticles());
  _csLocations.frameTimeDiff = _shaderManager.getUniformLocation(_computeProgID, "frameTimeDiff");
  _csLocations.pass = _shaderManager.getUniformLocation(_computeProgID, "pass");
   
  _shaderManager.useProgram(_shaderProgID);
  _shaderManager.loadUniform_(_shaderProgID, "quadLength", _quadLength);
  _shaderManager.loadMatrix4(_shaderProgID, "projMatrix", glm::value_ptr(_camera.getProjectionMatrix()));
  _psLocations.viewMatrix = _shaderManager.getUniformLocation(_shaderProgID, "viewMatrix");
  _psLocations.camPos = _shaderManager.getUniformLocation(_shaderProgID, "camPos");
  _psLocations.time = _shaderManager.getUniformLocation(_shaderProgID, "time");
  
  _shaderManager.resetProgram();

  _camera.setPosition(glm::vec4(15, 10, 15, 1));

  glEnable(GL_DEPTH_TEST);
  initDepthMap();

  // Depth test needs to be disabled for only rendering transparent particles. 
  //glDisable(GL_DEPTH_TEST);
  //glEnable(GL_BLEND);
  //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void ParticleSystem::resize(int width, int height){
  _camera.resize(width, height);
  _shaderManager.useProgram(_shaderProgID);
  _shaderManager.loadMatrix4(_shaderProgID, "projMatrix", glm::value_ptr(_camera.getProjectionMatrix()));
  _shaderManager.resetProgram();
}

void ParticleSystem::run(){
  
  using namespace std::chrono;
  
  bool running = true;
  
  Timer timer;
  timer.start = Timer::currentTime();
  timer.nextGameTick = Timer::currentTime();
  timer.lastFpsUpdate = timer.nextGameTick;
  timer.lastFrameRendered = timer.nextGameTick;
  std::string fpsStr;

  int cntDown = 10;
  
  while(running){

    timer.skippedFrames = 0;
    
    while(Timer::currentTime() > timer.nextGameTick && timer.skippedFrames < timer.maxFrameskip){
      _input.updateInput();
      
      // Exit the program if ESC is pressed
      if(_input.isKeyPressed(GLFW_KEY_ESCAPE)){
        running = false;
      }
      // Show FPS when TAB is pressed
      if(_input.isMouseButtonPressedOnce(GLFW_MOUSE_BUTTON_LEFT)){
        //_showFPS = !_showFPS;
        //if(!_showFPS) _window.setDefaultWindowTitle();
		  _tmpDeltaTime = _deltaTime;
		  _deltaTime = 0.1;
      }
	  else
	  {
		  _deltaTime = _tmpDeltaTime;
	  }
      // Enable/Disable VSync when Space is pressed
      if(_input.isKeyPressedOnce(GLFW_KEY_SPACE)){
        //_window.setVSync(!_window.isVSyncOn());
		  if (_deltaTime > 0.0f)
		  {
			  _deltaTime = 0.0f;
			  _tmpDeltaTime = _deltaTime;
		  }
		  else
		  {
			  _deltaTime = 0.01f;
			  _tmpDeltaTime = _deltaTime;
		  }
      }
      timer.nextGameTick += timer.skipTicks;
      timer.skippedFrames++;
    }
    
    auto dt = duration_cast<duration<double>>(Timer::currentTime() - timer.lastFrameRendered).count();
    timer.lastFrameRendered = Timer::currentTime();
    
	cntDown--;
	if (cntDown < 0)
	{
		cntDown = -1;
		_camera.updateCamera(dt, _input);
	}

    _attractor.updateAttractor(_camera, _input);
    auto currTime = duration_cast<milliseconds>(timer.start - Timer::currentTime()).count();
    render(dt, currTime);

    // Compute FPS and store them in a string.
    if (duration_cast<milliseconds>(Timer::currentTime() - timer.lastFpsUpdate) >= milliseconds(1000)) {
      fpsStr = std::to_string(timer.framesRendered);
      timer.lastFpsUpdate = Timer::currentTime();
      timer.framesRendered = 0;
    }
    timer.framesRendered++;

    if(_showFPS) _window.setWindowTitle(fpsStr);
    
    _window.swapBuffers();
  }
}

void ParticleSystem::render(double dt, double time){

  glBindVertexArray(_vertexArrayID);
  
  // Pass 1
  _shaderManager.useProgram(_computeProgID);
  _shaderManager.loadUniform(_csLocations.frameTimeDiff, _deltaTime);

  int pass_loc = glGetUniformLocation(_computeProgID, "pass");
  if (pass_loc != -1)
  {
	  glUniform1i(pass_loc, 1);
  }
 // _shaderManager.loadUniform(_csLocations.pass, 1);
  glDispatchCompute((_particleBuffer.getNumParticles()/WORK_GROUP_SIZE)+1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

  // Pass 2
  _shaderManager.useProgram(_computeProgID);
  _shaderManager.loadUniform(_csLocations.frameTimeDiff, _deltaTime);
  pass_loc = glGetUniformLocation(_computeProgID, "pass");
  if (pass_loc != -1)
  {
	  glUniform1i(pass_loc, 2);
  }
  glDispatchCompute((_particleBuffer.getNumParticles() / WORK_GROUP_SIZE) + 1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

  // Pass 3
  _shaderManager.useProgram(_computeProgID);
  _shaderManager.loadUniform(_csLocations.frameTimeDiff, _deltaTime);
  pass_loc = glGetUniformLocation(_computeProgID, "pass");
  if (pass_loc != -1)
  {
	  glUniform1i(pass_loc, 3);
  }
  glDispatchCompute((_particleBuffer.getNumParticles() / WORK_GROUP_SIZE) + 1, 1, 1);
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
  
  drawDepthMap(time);
  
  glClearColor(0.6, 0.6, 0.6, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  _shaderManager.useProgram(_shaderProgID);

  _shaderManager.loadMatrix4(_psLocations.viewMatrix, glm::value_ptr(_camera.getViewMatrix()));
  _shaderManager.loadUniform(_psLocations.camPos,
	  _camera.getPosition().x,
	  _camera.getPosition().y,
	  _camera.getPosition().z,
	  1.0f);

  _shaderManager.loadUniform(_psLocations.time, static_cast<GLfloat>(time));

  glDrawArrays(GL_POINTS, 0, _particleBuffer.getNumParticles());

  _shaderManager.resetProgram();

  glBindVertexArray(0);

  postProcessing();

}

void ParticleSystem::initDepthMap()
{
	glGenFramebuffers(1, &_depthMapFBO);

	glGenTextures(1, &_depthMap);
	glBindTexture(GL_TEXTURE_2D, _depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
		_window.getWidth(), _window.getHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	GLfloat borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ParticleSystem::drawDepthMap(double time)
{
	glBindFramebuffer(GL_FRAMEBUFFER, _depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);

	_shaderManager.useProgram(_shaderProgID);

	_shaderManager.loadMatrix4(_psLocations.viewMatrix, glm::value_ptr(_camera.getViewMatrix()));
	_shaderManager.loadUniform(_psLocations.camPos,
		_camera.getPosition().x,
		_camera.getPosition().y,
		_camera.getPosition().z,
		1.0f);

	_shaderManager.loadUniform(_psLocations.time, static_cast<GLfloat>(time));

	glDrawArrays(GL_POINTS, 0, _particleBuffer.getNumParticles());

	_shaderManager.resetProgram();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ParticleSystem::postProcessing() {

	glUseProgram(_ppShaderProgID);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, _depthMap);

	int depthMap_loc = glGetUniformLocation(_ppShaderProgID, "depthMap");
	if (depthMap_loc != -1)
	{
		glUniform1i(depthMap_loc, 0);
	}
	glBindVertexArray(_ppVertexArrayID);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void ParticleSystem::deleteParticleSystem() noexcept{
  if(_computeProgID)_shaderManager.deleteProgram(_computeProgID);
  if(_shaderProgID) _shaderManager.deleteProgram(_shaderProgID);
}
