#version 430
#extension GL_ARB_compute_shader : enable
#extension GL_ARB_shader_storage_buffer_object : enable

#define PI_FLOAT 3.1415927410125732421875f
#define PARTICLE_RADIUS 0.025f
#define PARTICLE_RESTING_DENSITY 1000
#define PARTICLE_MASS 65
#define SMOOTHING_LENGTH (24 * PARTICLE_RADIUS)
#define PARTICLE_VISCOSITY 250.f
#define GRAVITY_FORCE vec3(0, -10, 0)
#define PARTICLE_STIFFNESS 200
#define WALL_DAMPING 0.8f

struct particle{
  vec4  currPos;
  vec4  prevPos;
  vec4  acceleration;
  vec4  velocity;
  vec4  pamameters;
  vec4  deltaCs;
};

layout(std430, binding=0) buffer particles{
  particle p[];
};

uniform int pass;
uniform float frameTimeDiff;
uniform uint maxParticles;

layout (local_size_x = 256, local_size_y = 1, local_size_z = 1) in;



void main(){
  uint i = gl_GlobalInvocationID.x;
  
  if(i <= maxParticles)
  {
    
	// compute density and pressure per particle
    if(pass == 1)
    {
		int cnt = 0;
		float density_sum = 0.f;
		for (int j = 0; j < maxParticles; j++)
		{
			vec3 delta = (p[i].currPos - p[j].currPos).xyz;
			float r = length(delta);
			if (r < SMOOTHING_LENGTH)
			{
				cnt++;
				density_sum += PARTICLE_MASS * /* poly6 kernel */ 315.f * pow(SMOOTHING_LENGTH * SMOOTHING_LENGTH - r * r, 3) / (64.f * PI_FLOAT * pow(SMOOTHING_LENGTH, 9));
			}
		}
		p[i].pamameters[0] = density_sum;
		// compute pressure
		p[i].pamameters[1] = max(PARTICLE_STIFFNESS * (density_sum - PARTICLE_RESTING_DENSITY), 0.f);
		p[i].pamameters[2] = float(cnt); 
    } 

	else if(pass == 2)
    {
		// compute all forces
		vec3 pressure_force = vec3(0);
		vec3 viscosity_force = vec3(0);
		vec3 dCs = vec3(0);
    
		for (uint j = 0; j < maxParticles; j++)
		{
			if (i == j)
			{
				continue;
			}
			vec3 delta = (p[i].currPos - p[j].currPos).xyz;
			float r = length(delta);
			if (r < SMOOTHING_LENGTH)
			{
				pressure_force -= PARTICLE_MASS * (p[i].pamameters[1] + p[j].pamameters[1]) / (2.f * p[j].pamameters[0]) *
				// gradient of spiky kernel
                -45.f / (PI_FLOAT * pow(SMOOTHING_LENGTH, 6)) * pow(SMOOTHING_LENGTH - r, 2) * normalize(delta);

				viscosity_force += PARTICLE_MASS * (p[j].velocity - p[i].velocity).xyz / p[j].pamameters[0] *
				// Laplacian of viscosity kernel
                45.f / (PI_FLOAT * pow(SMOOTHING_LENGTH, 6)) * (SMOOTHING_LENGTH - r);

				dCs -= PARTICLE_MASS * pow(SMOOTHING_LENGTH * SMOOTHING_LENGTH - r * r, 2) / p[j].pamameters[0] *
				// Poly6 kernel 
				945.f / (32.f * PI_FLOAT * pow(SMOOTHING_LENGTH, 9)) * delta;
			}
		}
		viscosity_force *= PARTICLE_VISCOSITY;

		p[i].acceleration = vec4((pressure_force / p[i].pamameters[0] + viscosity_force / p[i].pamameters[0] + GRAVITY_FORCE), 1.0f);

		p[i].deltaCs = vec4(normalize(dCs), 1.0f);
	} 

	else if(pass == 3)
    {
		// integrate
		vec3 new_velocity = (p[i].velocity + frameTimeDiff * p[i].acceleration).xyz;
		vec3 new_position = p[i].currPos.xyz + frameTimeDiff * new_velocity;

		// boundary conditions
		
		if (new_position.y < -10)
		{
			new_position.y = -10;
			new_velocity.y *= -1 * WALL_DAMPING;
		}
		else if (new_position.y > 10)
		{
			new_position.y = 10;
			new_velocity.y *= -1 * WALL_DAMPING;
		}
		if (new_position.x < -8)
		{
			new_position.x = -8;
			new_velocity.x *= -1 * WALL_DAMPING;
		}
		else if (new_position.x > 8)
		{
			new_position.x = 8;
			new_velocity.x *= -1 * WALL_DAMPING;
		}
		if (new_position.z < -10)
		{
			new_position.z = -10;
			new_velocity.z *= -1 * WALL_DAMPING;
		}
		else if (new_position.z > 4)
		{
			new_position.z = 4;
			new_velocity.z *= -1 * WALL_DAMPING;
		}

		p[i].velocity = vec4(new_velocity, 1.0f);
		p[i].prevPos = p[i].currPos;
		p[i].currPos = vec4(new_position, 1.0f);
    } 
  }
}