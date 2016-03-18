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

function animatedCylinder(from_radius, to_radius, from, to, faces, scene)
{
	var path = cylinderPaths(from_radius, to_radius, from, to, faces);

	var mesh = BABYLON.Mesh.CreateRibbon("ribbon", path, false, false, 0, scene, true, BABYLON.Mesh.SINGLESIDE);

	var positions = new Float32Array(mesh.getVerticesData(BABYLON.VertexBuffer.PositionKind) );
	var indices = new Uint16Array(mesh.getIndices());
	var normals = new Float32Array(positions.length);

	mesh.update = function(from_, to_)
		{
			var paths = cylinderPaths(from_radius, to_radius, from_, to_, faces);

			var i = 0;
			for (var pl = 0; pl != 2; ++pl)
			{
				for (var face = 0; face <= faces; ++face)
				{
					var ding = paths[pl][face];

					positions[i] = ding.x; ++i;
					positions[i] = ding.y; ++i;
					positions[i] = ding.z; ++i;
				}
			}

			mesh.updateVerticesData(BABYLON.VertexBuffer.PositionKind, positions, true);

			BABYLON.VertexData.ComputeNormals(positions, indices, normals);

			mesh.updateVerticesData(BABYLON.VertexBuffer.NormalKind, normals);
		};

	return mesh;
}

function animated_player_from_array(position, material, scene)
{
	var segment_updaters = [];

	for (var i = 0; i != segments.length; ++i)
	{
		if (segments[i][3])
		{
			var j_from = segments[i][0][0];
			var j_to = segments[i][0][1];

			var radius_to = joints[j_to][0];
			var radius_from = joints[j_from][0];
			var radius_center = segments[i][2];

			var from = position[j_from];
			var to = position[j_to];
			var center = from.add(to).scale(0.5);

			var ac = animatedCylinder(radius_from, radius_center, from, center, 16, scene);
			var ac2 = animatedCylinder(radius_center, radius_to, center, to, 16, scene);
			ac.material = material;
			ac2.material = material;

			segment_updaters.push(function(ac_, ac2_) { return function(from, to)
				{
					var center = from.add(to).scale(0.5);
					ac_.update(from, center);
					ac2_.update(center, to);
				};
				}(ac, ac2));
		}
		else
			segment_updaters.push(null);
	}

	var spheres = [];

	for (var j = 0; j != joints.length; ++j)
	{
		var sphere = BABYLON.Mesh.CreateSphere('sphere1',
			8, // faces
			joints[j][0] * 2, // diameter
			scene);

		sphere.material = material
		sphere.position = position[j];

		spheres.push(sphere);
	}

	return function(player)
	{
		for (var j = 0; j != joints.length; ++j)
			spheres[j].position = player[j];

		for (var s = 0; s != segments.length; ++s)
			if (segment_updaters[s] != null)
			{
				var j_from = segments[s][0][0];
				var j_to = segments[s][0][1];

				var from = player[j_from];
				var to = player[j_to];
				segment_updaters[s](from, to);
			}
	}
}

var speed = 200;

function animated_position_from_array(frame, scene)
{
	var skins =
		[ new BABYLON.StandardMaterial("redskin", scene)
		, new BABYLON.StandardMaterial("blueskin", scene) ];

	skins[0].diffuseColor = new BABYLON.Color3(0.1,0,0);
	skins[1].diffuseColor = new BABYLON.Color3(0,0,0.1);

	skins[0].specularColor = new BABYLON.Color3.Red;
	skins[1].specularColor = new BABYLON.Color3.Blue;

	skins[0].specularPower = 0;
	skins[1].specularPower = 0;

	var updaters = [];
	for (var pl = 0; pl != 2; ++pl)
		updaters.push(animated_player_from_array(frame[0], skins[pl], scene));

	var grey = new BABYLON.Color3(0.7, 0.7, 0.7);

	for (var i = -4; i <= 4; ++i)
	{
		var line = BABYLON.Mesh.CreateLines("l", [v3(i/2, 0, -2), v3(i/2, 0, 2)], scene);
		line.color = grey;

		line = BABYLON.Mesh.CreateLines("l", [v3(-2, 0, i/2), v3(2, 0, i/2)], scene);
		line.color = grey;
	}

	return function(p)
		{
			for (var pl = 0; pl != 2; ++pl)
				updaters[pl](p[pl]);
		};
}

function mirror(p)
{
	for (var pl = 0; pl != 2; ++pl)
	for (var j = 0; j != joints.length; ++j)
		p[pl][j].x = -p[pl][j].x;

	swapLimbs(p[0]);
	swapLimbs(p[1]);
}

Array.prototype.swap = function(x,y) {
  var b = this[x];
  this[x] = this[y];
  this[y] = b;
  return this;
}

function swapLimbs(p)
{
	p.swap(LeftShoulder, RightShoulder);
	p.swap(LeftElbow, RightElbow);
	p.swap(LeftHand, RightHand);
	p.swap(LeftFingers, RightFingers);
	p.swap(LeftWrist, RightWrist);
	p.swap(LeftAnkle, RightAnkle);
	p.swap(LeftToe, RightToe);
	p.swap(LeftHip, RightHip);
	p.swap(LeftHeel, RightHeel);
	p.swap(LeftKnee, RightKnee);
}

function apply_reo(r, p)
{
	var q = [[],[]];

	for (var pl = 0; pl != 2; ++pl)
	for (var j = 0; j != joints.length; ++j)
		q[pl].push(yrot(r.angle, p[pl][j]).add(r.offset));

	if (r.mirror)
		mirror(q);

	if (r.swap_players) q = [q[1], q[0]];

	return q;
}

function yrot(a, v)
{
	return v3
		( Math.cos(a) * v.x + Math.sin(a) * v.z
		, v.y
		, - Math.sin(a) * v.x + Math.cos(a) * v.z
		);
}

function inverse_reo(x)
{
	var c =
		{ offset: yrot(-x.angle, x.offset.negate())
		, angle: -x.angle
		, mirror: x.mirror
		, swap_players: x.swap_players
		};

	if (x.mirror)
	{
		c.angle = -c.angle;
		c.offset.x = -c.offset.x;
	}

	return c;
}

function compose_reo(a, b)
{
	var c =
		{ mirror: a.mirror != b.mirror
		, swap_players: a.swap_players != b.swap_players
		};

	if (a.mirror)
	{
		var boff = v3(-b.offset.x, b.offset.y, b.offset.z);

		c.angle = a.angle - b.angle;
		c.offset = boff.add(yrot(-b.angle, a.offset));
	}
	else
	{
		c.angle = a.angle + b.angle;
		c.offset = b.offset.add(yrot(b.angle, a.offset));
	}

	return c;
}

