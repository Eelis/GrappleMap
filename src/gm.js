function cylinder(from, to, from_radius, to_radius, faces, scene)
{
	var s = (Math.PI * 2) / faces;

	var a = BABYLON.Vector3.Cross(to.subtract(from), v3(1,1,1).subtract(from)).normalize();
	var b = BABYLON.Vector3.Cross(to.subtract(from), a).normalize();

	var p0 = [];
	var p1 = [];

	for (var i = faces; i >= 0; --i)
	{
		p0.push(from
			.add(a.scale(from_radius * Math.sin(i * s)))
			.add(b.scale(from_radius * Math.cos(i * s))));

		p1.push(to
			.add(a.scale(to_radius * Math.sin(i * s)))
			.add(b.scale(to_radius * Math.cos(i * s))));
	}

	var r = BABYLON.Mesh.CreateRibbon('jimmy',
		[p0, p1],
		false, // closeArray
		false, // closePath
		0,
		scene,
		true);
	
	return r;
}

LeftToe = 0
RightToe = 1
LeftHeel = 2
RightHeel = 3
LeftAnkle = 4
RightAnkle = 5
LeftKnee = 6
RightKnee = 7
LeftHip = 8
RightHip = 9
LeftShoulder = 10
RightShoulder = 11
LeftElbow = 12
RightElbow = 13
LeftWrist = 14
RightWrist = 15
LeftHand = 16
RightHand = 17
LeftFingers = 18
RightFingers = 19
Core = 20
Neck = 21
Head = 22

function v3(x,y,z){ return new BABYLON.Vector3(x, y, z); }

var segments =
	[ [[LeftToe, LeftHeel], 0.23, 0.025, true]
	, [[LeftToe, LeftAnkle], 0.18, 0.025, true]
	, [[LeftHeel, LeftAnkle], 0.09, 0.025, true]
	, [[LeftAnkle, LeftKnee], 0.42, 0.055, true]
	, [[LeftKnee, LeftHip], 0.44, 0.085, true]
	, [[LeftHip, Core], 0.27, 0.1, true]
	, [[Core, LeftShoulder], 0.37, 0.075, true]
	, [[LeftShoulder, LeftElbow], 0.29, 0.06, true]
	, [[LeftElbow, LeftWrist], 0.26, 0.03, true]
	, [[LeftWrist, LeftHand], 0.08, 0.02, true]
	, [[LeftHand, LeftFingers], 0.08, 0.02, true]
	, [[LeftWrist, LeftFingers], 0.14, 0.02, false]
	, [[RightToe, RightHeel], 0.23, 0.025, true]
	, [[RightToe, RightAnkle], 0.18, 0.025, true]
	, [[RightHeel, RightAnkle], 0.09, 0.025, true]
	, [[RightAnkle, RightKnee], 0.42, 0.055, true]
	, [[RightKnee, RightHip], 0.44, 0.085, true]
	, [[RightHip, Core], 0.27, 0.1, true]
	, [[Core, RightShoulder], 0.37, 0.075, true]
	, [[RightShoulder, RightElbow], 0.29, 0.06, true]
	, [[RightElbow, RightWrist], 0.27, 0.03, true]
	, [[RightWrist, RightHand], 0.08, 0.02, true]
	, [[RightHand, RightFingers], 0.08, 0.02, true]
	, [[RightWrist, RightFingers], 0.14, 0.02, false]
	, [[LeftHip, RightHip], 0.23, 0.1,  false]
	, [[LeftShoulder, Neck], 0.175, 0.065, true]
	, [[RightShoulder, Neck], 0.175, 0.065, true]
	, [[Neck, Head], 0.165, 0.05, true]
	];

var joints =
	[ [0.025, false]
	, [0.025, false]
	, [0.03, false]
	, [0.03, false]
	, [0.03, true]
	, [0.03, true]
	, [0.05, true]
	, [0.05, true]
	, [0.09, true]
	, [0.09, true]
	, [0.08, true]
	, [0.08, true]
	, [0.045, true]
	, [0.045, true]
	, [0.02, false]
	, [0.02, false]
	, [0.02, true]
	, [0.02, true]
	, [0.02, false]
	, [0.02, false]
	, [0.1, false]
	, [0.05, false]
	, [0.11, true]
	];

function player_from_array(a, material, scene)
{
	for (var i = 0; i != segments.length; ++i)
	{
		if (segments[i][3])
		{
			var j_from = segments[i][0][0];
			var j_to = segments[i][0][1];

			var radius_to = joints[j_to][0];
			var radius_from = joints[j_from][0];

			var pos_from = a[j_from];
			var pos_to = a[j_to];

			var center = pos_from.add(pos_to).scale(0.5);

			var radius_center = segments[i][2];

			var c_from = cylinder(pos_from, center, radius_from, radius_center, 32, scene);
			var c_to = cylinder(center, pos_to, radius_center, radius_to, 32, scene);

			c_from.material = material;
			c_to.material = material;
		}
	}

	for (var i = 0; i != joints.length; ++i)
	{
		var sphere = BABYLON.Mesh.CreateSphere('sphere1',
			16, // faces
			joints[i][0] * 2, // diameter
			scene);

		sphere.material = material
		sphere.position = a[i];
	}
}




function cylinderPaths(from_radius, to_radius, from, to, faces)
{
    var p0 = [];
	var p1 = [];
	var a = BABYLON.Vector3.Cross(to.subtract(from), (new BABYLON.Vector3(1,1,1)).subtract(from)).normalize();
	var b = BABYLON.Vector3.Cross(to.subtract(from), a).normalize();
	var s = (Math.PI * 2) / faces;

	for (var i = faces; i >= 0; --i)
	{
		p0.push(from
			.add(a.scale(from_radius * Math.sin(i * s)))
			.add(b.scale(from_radius * Math.cos(i * s))));

		p1.push(to
			.add(a.scale(to_radius * Math.sin(i * s)))
			.add(b.scale(to_radius * Math.cos(i * s))));
	}

	return [p0, p1];
}

var drag = 0.25;

var frame = 0;
var k = 0;

function bound_frame_index(i, n)
{
	if (i < 0) return 0;
	if (i > n - 1) return n - 1;
	return i;
}

function animatedCylinder(from_radius, to_radius, froms, tos, faces, scene)
{
	var paths = [];

	for (var i = 0; i != froms.length; ++i)
		paths.push(cylinderPaths(from_radius, to_radius, froms[i], tos[i], faces));

	var mesh = BABYLON.Mesh.CreateRibbon("ribbon", paths[0], false, false, 0, scene, true, BABYLON.Mesh.SINGLESIDE);

	var positions = new Float32Array(mesh.getVerticesData(BABYLON.VertexBuffer.PositionKind) );
	var indices = new Uint16Array(mesh.getIndices());
	var normals = new Float32Array(positions.length);

	if (froms.length > 1)
	{
		var dragged_from = froms[0];
		var dragged_to = tos[0];

		function tick()
		{
			var this_frame = bound_frame_index(frame, froms.length);
			var next_frame = bound_frame_index(frame + 1, froms.length);

			var current_from = froms[this_frame].scale(1-k).add(froms[next_frame].scale(k));
			var current_to = tos[this_frame].scale(1-k).add(tos[next_frame].scale(k));

			dragged_from = dragged_from.scale(1-drag).add(current_from.scale(drag));
			dragged_to = dragged_to.scale(1-drag).add(current_to.scale(drag));

			var some_paths = cylinderPaths(from_radius, to_radius, dragged_from, dragged_to, faces);

			var i = 0;
			for (var pl = 0; pl != 2; ++pl)
			{
				for (var face = 0; face <= faces; ++face)
				{
					var ding = some_paths[pl][face];

					positions[i] = ding.x; ++i;
					positions[i] = ding.y; ++i;
					positions[i] = ding.z; ++i;
				}
			}

			mesh.updateVerticesData(BABYLON.VertexBuffer.PositionKind, positions);

			BABYLON.VertexData.ComputeNormals(positions, indices, normals);

			mesh.updateVerticesData(BABYLON.VertexBuffer.NormalKind, normals);
		}

		scene.registerBeforeRender(tick);
	}

	return mesh;
}


function animated_player_from_array(aa, pl, material, scene)
{
	for (var i = 0; i != segments.length; ++i)
	{
		if (segments[i][3])
		{
			var j_from = segments[i][0][0];
			var j_to = segments[i][0][1];

			var radius_to = joints[j_to][0];
			var radius_from = joints[j_from][0];
			var radius_center = segments[i][2];

			var pos_froms = [];
			var centers = [];
			var pos_tos = [];

			for (var u = 0; u != aa.length; ++u)
			{
				var from = aa[u][pl][j_from];
				var to = aa[u][pl][j_to];
				var center = from.add(to).scale(0.5);

				pos_froms.push(from);
				centers.push(center);
				pos_tos.push(to);
			}

			var ac = animatedCylinder(radius_from, radius_center, pos_froms, centers, 16, scene);
			var ac2 = animatedCylinder(radius_center, radius_to, centers, pos_tos, 16, scene);
			ac.material = material;
			ac2.material = material;
		}
	}

	var spheres = [];

	for (var i = 0; i != joints.length; ++i)
	{
		var sphere = BABYLON.Mesh.CreateSphere('sphere1',
			8, // faces
			joints[i][0] * 2, // diameter
			scene);

		sphere.material = material
		sphere.position = aa[0][pl][i];

		spheres.push(sphere);
	}

	if (aa.length > 1)
	{
		function tick()
		{
			for (var j = 0; j != joints.length; ++j)
			{
				var t = aa[bound_frame_index(frame, aa.length)][pl][j].scale(1 - k).add(aa[bound_frame_index(frame + 1, aa.length)][pl][j].scale(k));

				spheres[j].position = t.scale(drag).add(spheres[j].position.scale(1 - drag));
			}
		}

		scene.registerBeforeRender(tick);
	}
}

function position_from_array(a, scene)
{
	var redskin = new BABYLON.StandardMaterial("redskin", scene);
	var blueskin = new BABYLON.StandardMaterial("blueskin", scene);

	redskin.diffuseColor = new BABYLON.Color3(0.1,0,0);
	blueskin.diffuseColor = new BABYLON.Color3(0,0,0.1);

	redskin.specularColor = new BABYLON.Color3.Red;
	blueskin.specularColor = new BABYLON.Color3.Blue;

	blueskin.specularPower = 0;
	redskin.specularPower = 0;

	player_from_array(a[0], redskin, scene);
	player_from_array(a[1], blueskin, scene);

	var grey = new BABYLON.Color3(0.7, 0.7, 0.7);

	for (var i = -4; i <= 4; ++i)
	{
		var line = BABYLON.Mesh.CreateLines("l", [v3(i/2, 0, -2), v3(i/2, 0, 2)], scene);
		line.color = grey;

		line = BABYLON.Mesh.CreateLines("l", [v3(-2, 0, i/2), v3(2, 0, i/2)], scene);
		line.color = grey;
	}
}

var speed = 200;

function animated_position_from_array(aa, scene)
{
	var redskin = new BABYLON.StandardMaterial("redskin", scene);
	var blueskin = new BABYLON.StandardMaterial("blueskin", scene);

	redskin.diffuseColor = new BABYLON.Color3(0.1,0,0);
	blueskin.diffuseColor = new BABYLON.Color3(0,0,0.1);

	redskin.specularColor = new BABYLON.Color3.Red;
	blueskin.specularColor = new BABYLON.Color3.Blue;

	blueskin.specularPower = 0;
	redskin.specularPower = 0;

	animated_player_from_array(aa, 0, redskin, scene);
	animated_player_from_array(aa, 1, blueskin, scene);

	var grey = new BABYLON.Color3(0.7, 0.7, 0.7);

	for (var i = -4; i <= 4; ++i)
	{
		var line = BABYLON.Mesh.CreateLines("l", [v3(i/2, 0, -2), v3(i/2, 0, 2)], scene);
		line.color = grey;

		line = BABYLON.Mesh.CreateLines("l", [v3(-2, 0, i/2), v3(2, 0, i/2)], scene);
		line.color = grey;
	}
}

function showpos(a, engine)
{
	var canvas = document.getElementById('renderCanvas');

	var scene = new BABYLON.Scene(engine);

	scene.clearColor = new BABYLON.Color3(1,1,1);

	var camera = new BABYLON.ArcRotateCamera("ArcRotateCamera",
		0, // rotation around Y axis
		3.14/4, // rotation around X axis
		2.5, // radius
		new BABYLON.Vector3(0, 0, 0),
		scene);

	camera.wheelPrecision = 30;
	camera.lowerBetaLimit = 0;
	camera.upperRadiusLimit = 4;
	camera.lowerRadiusLimit = 1;

	camera.attachControl(canvas, false);

	var light = new BABYLON.HemisphericLight('light1', new BABYLON.Vector3(0,1,0), scene);

	animated_position_from_array(a, scene);

	return scene;
}
