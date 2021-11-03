// Vertex Shader

attribute vec4 aPosition; // vertex position
attribute vec3 aNormal; // normal vector

uniform mat4 uWorldMatrix; // world coordinate
uniform mat4 uModelMatrix; // object coordinate
uniform mat4 uPerspectiveMatrix; // perspective

// Phong light model
uniform vec3 uLightPosition;
uniform float uShiness;
uniform vec4 uAmbientProduct, uDiffuseProduct, uSpecularProduct;

// attribute vec4 aColor;
uniform vec4 uColor;

varying vec4 vColor;
varying vec4 vLight;

void main() {
	// vertex absolute position
	vec3 posToWorld = (uWorldMatrix * aPosition).xyz;
	
	// calculate Phong light model
	vec3 lightPos = uLightPosition.xyz;
	vec3 L = normalize(lightPos - posToWorld);
	vec3 E = -normalize(posToWorld);
	vec3 H = normalize(L + E);
	vec4 NN = vec4(aNormal, 1.0);
	// avoid normal change
	vec3 N = normalize((mat3(transpose(inverse(uWorldMatrix))) * NN.xyz).xyz);
	// calculate 3 components
	vec4 ambient = uAmbientProduct;
	float Kd = max(dot(L, N), 0.0);
	vec4 diffuse = Kd * uDiffuseProduct;
	float Ks = pow(max(dot(N, H), 0.0), uShiness);
	vec4 specular = Ks * uSpecularProduct;
	if (dot(L, N) < 0.0) {
		specular = vec4(0.0, 0.0, 0.0, 1.0);
	}

	gl_Position = uPerspectiveMatrix * uWorldMatrix * uModelMatrix * aPosition;

	// convey to fragment shader
	vColor = uColor;
	vLight = vec4((ambient + diffuse + specular).rgb, 1.0);
}