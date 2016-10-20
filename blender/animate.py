import bpy
from mathutils import Matrix, Vector
from math import sin, cos

def ncross(a,b):
    return a.normalized().cross(b.normalized())

def ortho(a,b,c): # returns a vector orthogonal to both (b-a) and (c-a)
    return ncross(b - a, c - a)

def lerp(a,b,c=0.5):
    return a * (1 - c) + b * c

def assignBone(poseBone, matrix, armaBone, scale):
    mi = armaBone.matrix.copy()
    mi.invert()
    poseBone.rotation_mode = 'XYZ'
    poseBone.rotation_euler = (mi * matrix).to_euler()
    poseBone.scale = Vector((scale,scale,scale))

dump = open('/home/eelis/projects/GrappleMap/dev/dump').read()[:-2]
d = list(map(float, dump.split(' ')))

di = 0

def readvec():
    global di
    r = Vector((d[di], d[di+1], d[di+2]))
    di += 3
    return r

def readPlayer():
    pos = {}
    pos['core'] = readvec()
    pos['neck'] = readvec()
    pos['head'] = readvec()
    pos['lefthip'] = readvec()
    pos['leftknee'] = readvec()
    pos['leftankle'] = readvec()
    pos['leftheel'] = readvec()
    pos['lefttoe'] = readvec()
    pos['leftshoulder'] = readvec()
    pos['leftelbow'] = readvec()
    pos['leftwrist'] = readvec()
    pos['lefthand'] = readvec()
    pos['leftfingers'] = readvec()
    pos['righthip'] = readvec()
    pos['rightknee'] = readvec()
    pos['rightankle'] = readvec()
    pos['rightheel'] = readvec()
    pos['righttoe'] = readvec()
    pos['rightshoulder'] = readvec()
    pos['rightelbow'] = readvec()
    pos['rightwrist'] = readvec()
    pos['righthand'] = readvec()
    pos['rightfingers'] = readvec()
    sideways = (pos['righthip'] - pos['lefthip']).normalized()
    pos['lefthip'] -= sideways * 0.01
    pos['righthip'] += sideways * 0.01
    pos['lefthip'] = lerp(pos['lefthip'], pos['core'], 0.2)
    pos['righthip'] = lerp(pos['righthip'], pos['core'], 0.2)
    pos['hips'] = lerp(pos['lefthip'], pos['righthip'])
    pos['core'] = lerp(pos['core'], pos['neck'], 0.12)
    pos['neck'] = lerp(pos['neck'], pos['head'], 0.25)

    # todo: do these nasty ad-hoc adjustments properly

    return pos

positions = []

while di != len(d):
    red = readPlayer()
    blue = readPlayer()
    positions.append([red, blue])

arma = bpy.data.armatures['redplayer']

def cmu_enrich(p):
    p['cmu_hips'] = lerp(p['lefthip'], p['righthip'])
    hipsfwd = ortho(p['cmu_hips'], p['core'], p['righthip'])
    corefwd = ortho(p['core'], p['leftshoulder'], p['rightshoulder'])

    p['cmu_lowerback'] = lerp(p['cmu_hips'], p['core'], 0.35) + hipsfwd * -0.04
    p['cmu_spine1'] = lerp(p['core'], p['neck'], 0.4) + corefwd * -0.07
    p['cmu_neck1'] = lerp(p['head'], p['neck'])

    spine1up = (p['neck'] - p['cmu_spine1']).normalized()
    spine1fwd = ortho(p['cmu_spine1'], p['leftshoulder'], p['rightshoulder'])
    spine1left = ncross(spine1fwd, spine1up)

    spine1_resize = (p['neck'] - p['cmu_spine1']).length / (arma.bones['Neck'].head.length / 10)

    mm = Matrix((
        (spine1left.x,spine1up.x,spine1fwd.x),
        (spine1left.y,spine1up.y,spine1fwd.y),
        (spine1left.z,spine1up.z,spine1fwd.z)))

    mm2 = Matrix((
        (-spine1left.x,spine1up.x,spine1fwd.x),
        (-spine1left.y,spine1up.y,spine1fwd.y),
        (-spine1left.z,spine1up.z,spine1fwd.z)))

    p['cmu_leftshoulder'] = p['cmu_spine1'] + (mm2 * (0.1 * spine1_resize * arma.bones['LeftShoulder'].head))
    p['cmu_rightshoulder'] = p['cmu_spine1'] + (mm2 * (0.1 * spine1_resize * arma.bones['RightShoulder'].head))
    p['cmu_righthand'] = lerp(p['rightwrist'], p['rightelbow'], 0.1)
    p['cmu_lefthand'] = lerp(p['leftwrist'], p['leftelbow'], 0.1)
    p['cmu_spine'] = p['core'] + corefwd * -0.05

    backofhand = ncross(
        ortho(p['rightwrist'], p['rightfingers'], p['righthand']),
        p['rightwrist'] -  p['cmu_righthand'])
    handup = (p['rightwrist'] - p['cmu_righthand']).normalized()
    handleft = backofhand
    handfwd = -ncross(handup, handleft)

    mm = Matrix((
        (handleft.x,handup.x,handfwd.x),
        (handleft.y,handup.y,handfwd.y),
        (handleft.z,handup.z,handfwd.z)))
    rhand_resize = (p['rightwrist'] - p['cmu_righthand']).length / (arma.bones['RightFingerBase'].head.length / 10)

    p['cmu_rightthumb'] = p['cmu_righthand'] + (mm * (0.1 * rhand_resize * arma.bones['RThumb'].head))


    backofhand = ncross(
        ortho(p['leftwrist'], p['leftfingers'], p['lefthand']),
        p['leftwrist'] -  p['cmu_lefthand'])
    handup = (p['leftwrist'] - p['cmu_lefthand']).normalized()
    handleft = -backofhand
    handfwd = -ncross(handup, handleft)

    mm = Matrix((
        (handleft.x,handup.x,handfwd.x),
        (handleft.y,handup.y,handfwd.y),
        (handleft.z,handup.z,handfwd.z)))
    lhand_resize = (p['leftwrist'] - p['cmu_lefthand']).length / (arma.bones['LeftFingerBase'].head.length / 10)

    p['cmu_leftthumb'] = p['cmu_lefthand'] + (mm * (0.1 * lhand_resize * arma.bones['LThumb'].head))


    # todo: this is all horrible horrible horrible, clean up!


def gm_to_cmu(p): # p must be a grapplemap position

    cmu_enrich(p)

    lowerback = p['cmu_lowerback']
    spine1 = p['cmu_spine1']
    neck1 = p['cmu_neck1']
    leftshoulder = p['cmu_leftshoulder']
    rightshoulder = p['cmu_rightshoulder']
    spine = p['cmu_spine']

    cmu = {}

    ## HIPS

    fwdR = ortho(p['cmu_lowerback'], p['righthip'], p['lefthip'])
    ll = ncross(p['cmu_lowerback'] - p['hips'], fwdR)
    cmu['Hips'] = (p['cmu_lowerback'] - p['hips'], ll)


    ## SPINE

    fff = ortho(p['cmu_lowerback'], p['righthip'], p['lefthip'])
    lll = ncross(p['cmu_spine'] - p['cmu_lowerback'], fff)

    lolfwd = ortho(p['cmu_spine'], p['leftshoulder'], p['rightshoulder'])
    lolleft = -ncross(lolfwd, p['cmu_spine'] - p['cmu_lowerback'])

    cmu['LowerBack'] = (p['cmu_spine'] - p['cmu_lowerback'], ((lll + lolleft) / 2).normalized())

    newfwd = ortho(p['cmu_spine'], p['leftshoulder'], p['rightshoulder'])
    newleft = ncross(spine1 - p['cmu_spine'], newfwd)

    cmu['Spine'] = (
        spine1 - p['cmu_spine'],
        newleft
        )
    cmu['Spine1'] = (
        p['neck'] - spine1,
        ncross(p['neck'] - p['cmu_spine1'], ortho(p['cmu_spine1'], p['leftshoulder'], p['rightshoulder'])))

    neckfwd = ortho(p['neck'], rightshoulder, leftshoulder)
    neckleft = ncross(neck1 - p['neck'], neckfwd)
    cmu['Neck'] = (neck1 - p['neck'], neckleft)

    cmu['Neck1'] = (p['head'] - neck1, neckleft)

    # LEFT LEG

    cmu['LHipJoint'] = (
        p['lefthip'] - p['hips'],
        -fwdR)
    cmu['LeftUpLeg'] = (
        p['leftknee'] - p['lefthip'],
        ncross(
            ortho(p['lefthip'], p['righthip'], p['leftknee']),
            p['leftknee'] - p['lefthip']))
    cmu['LeftLeg'] = (
        p['leftankle'] - p['leftknee'],
        ortho(p['leftknee'], p['leftankle'], p['lefthip']))
    cmu['LeftFoot'] = (
        p['lefttoe'] - p['leftankle'],
        ortho(p['leftankle'], p['lefttoe'], p['leftheel']))

    # RIGHT LEG

    cmu['RHipJoint'] = (
        p['righthip'] - p['hips'],
        fwdR)
    cmu['RightUpLeg'] = (
        p['rightknee'] - p['righthip'],
        -ncross(
            ortho(p['righthip'], p['lefthip'], p['rightknee']),
            p['rightknee'] - p['righthip']))
    cmu['RightLeg'] = (
        p['rightankle'] - p['rightknee'],
        ortho(p['rightknee'], p['rightankle'], p['righthip']))
    cmu['RightFoot'] = (
        p['righttoe'] - p['rightankle'],
        ortho(p['rightankle'], p['righttoe'], p['rightheel']))

    # LEFT ARM

    cmu['LeftShoulder'] = (
        p['leftshoulder'] - p['cmu_leftshoulder'],
        ncross(p['leftshoulder'] - leftshoulder, ortho(leftshoulder, p['leftshoulder'], p['neck'])))
    cmu['LeftArm'] = (
        p['leftelbow'] - p['leftshoulder'],
        ortho(p['leftelbow'], p['leftwrist'], p['leftshoulder']))
    backofhand = ncross(
            p['leftwrist'] - p['cmu_lefthand'],
        ortho(p['leftwrist'], p['leftfingers'], p['lefthand']))
    cmu['LeftForeArm'] = (p['cmu_lefthand'] - p['leftelbow'], backofhand)
    cmu['LeftHand'] = (p['leftwrist'] - p['cmu_lefthand'], backofhand)
    cmu['LeftFingerBase'] = (
        p['lefthand']- p['leftwrist'],
        ortho(p['lefthand'], p['leftfingers'], p['leftwrist']))
    cmu['LeftHandFinger1'] = (
        p['leftfingers'] - p['lefthand'],
        ortho(p['lefthand'], p['leftfingers'], p['leftwrist']))
    # todo: thumb

    # RIGHT ARM

    cmu['RightShoulder'] = (
        p['rightshoulder'] - rightshoulder,
        ncross(ortho(rightshoulder, p['rightshoulder'], p['neck']), p['rightshoulder'] - rightshoulder))
    cmu['RightArm'] = (
        p['rightelbow'] - p['rightshoulder'],
        ortho(p['rightelbow'], p['rightwrist'], p['rightshoulder']))
    backofhand = ncross(
        ortho(p['rightwrist'], p['rightfingers'], p['righthand']),
        p['rightwrist'] -  p['cmu_righthand'])
    cmu['RightForeArm'] = ( p['cmu_righthand'] - p['rightelbow'], backofhand)
    cmu['RightHand'] = (p['rightwrist'] - p['cmu_righthand'], backofhand)
    cmu['RightFingerBase'] = (
        p['righthand'] - p['rightwrist'],
        ortho(p['righthand'], p['rightfingers'], p['rightwrist']))
    cmu['RightHandFinger1'] = (
        p['rightfingers'] - p['righthand'],
        ortho(p['righthand'], p['rightfingers'], p['rightwrist']))
    # todo: thumb

    return cmu


def orientBone(bone, m, arma, gm_tonext, toscale):

    armaBone = arma.bones[bone.name]

    if bone.name in gm_tonext:

        (raw_tonext, raw_left) = gm_tonext[bone.name]

        if bone.name == 'Neck1' or bone.name == 'Neck':
            resize = 1
        elif armaBone.length > 0.1:
            resize = (raw_tonext.length * 10) / armaBone.length
        elif bone.name == 'Hips':
            lowerback_trans = arma.bones['LowerBack'].head
            resize = raw_tonext.length / (lowerback_trans.length / 10)
        elif bone.name == 'Spine1':
            resize = raw_tonext.length / (arma.bones['Neck'].head.length / 10)
        elif bone.name == "LeftHand":
            resize = raw_tonext.length / (arma.bones['LeftFingerBase'].head.length / 10)
        elif bone.name == "RightHand":
            resize = raw_tonext.length / (arma.bones['RightFingerBase'].head.length / 10)
        else:
            resize = 1

        # todo: document the silly reason for the above trickery

        localscale = toscale * resize

        tonext = m * raw_tonext
        up = tonext.normalized()

        leftP = raw_left.normalized()

        left = m * leftP
        fwd = left.cross(up)

        M_up3 = Matrix((
          (left.x, up.x, fwd.x),
          (left.y, up.y, fwd.y),
          (left.z, up.z, fwd.z)))

        M_down3 = M_up3.copy(); M_down3.invert()


        assignBone(bone, M_up3, armaBone, toscale * resize)

        m = M_down3 * m

    else:
        assignBone(bone, Matrix.Identity(3), armaBone, 1)

    for c in bone.children:
        orientBone(c, m, arma, gm_tonext, 1 / resize)

def orientPlayer(pos, obj, arma):

    rootPoseBone = obj.pose.bones['Hips']
    rootArmaBone = arma.bones['Hips']

    mm = rootArmaBone.matrix.copy()
    mi = mm.copy()
    mi.invert()

    lowerback = (pos['hips'] + pos['core']) / 2.

    orientBone(rootPoseBone, Matrix.Identity(3), arma, gm_to_cmu(pos), 1)

    hmz = rootArmaBone.matrix_local
    rootTrans = Vector((hmz[0][3], hmz[1][3], hmz[2][3]))

    rootPoseBone.location = mi * (-rootTrans + (pos['hips'] * 10))

add_keyframes = False
do_render = True
render_start = 0

scene = bpy.context.scene

redobj = bpy.data.objects['redplayer']
redarma = bpy.data.armatures['redplayer']

blueobj = bpy.data.objects['blueplayer']
bluearma = bpy.data.armatures['blueplayer']

lamp = bpy.data.objects['Lamp']

old_frame = scene.frame_current

jump_duration = 110

# script will malfunction if there's keyframes in its way

camera = bpy.data.objects['camera']
lightoff = Vector((0, 0, 8))
initial_pos = positions[0]

banner_height = 4.47 # todo: query


def set_cam(center, rot):

    camera.rotation_mode = 'XYZ'
    camera.rotation_euler.z = rot
    camera.rotation_euler.x = 1.25
    camera.rotation_euler.y = 0

    campos = center + Vector((sin(rot) * 4, 1, cos(rot) * 4))

    camera.location.x = campos.x
    camera.location.y = -campos.z
    camera.location.z = campos.y


def key(pos):
    scene.objects.active = redobj
    orientPlayer(pos[0], redobj, redarma)

    if add_keyframes: bpy.ops.anim.keyframe_insert_menu(type='WholeCharacter')

    scene.objects.active = blueobj
    orientPlayer(pos[1], blueobj, bluearma)

    if add_keyframes: bpy.ops.anim.keyframe_insert_menu(type='WholeCharacter')

def render():
    if do_render and bpy.context.scene.frame_current >= render_start:
        bpy.context.scene.render.filepath = "//frames/" + str(bpy.context.scene.frame_current).zfill(5)
        bpy.ops.render.render(write_still=True)

def movelight(to):
    lamp.location.x = to.x
    lamp.location.y = -to.z
    lamp.location.z = 8

    if add_keyframes:
        lamp.keyframe_insert(data_path="location", frame=scene.frame_current)

## WAIT

set_cam(Vector((0, banner_height-1, 0)), 0)
scene.frame_current = 0
camera_center = Vector((0, ((positions[jump_duration][0]['core'] + positions[jump_duration][1]['core']) / 2).y, 0))
movelight(camera_center + lightoff)
for bla in range(100):
    if add_keyframes:
        camera.keyframe_insert(data_path="location", frame=scene.frame_current)
        camera.keyframe_insert(data_path="rotation_euler", frame=scene.frame_current)
    render()
    scene.frame_current += 1

## JUMP

jump_size = banner_height - (camera_center.y + 1)
i = 0

for bla in range(jump_duration):
    camera.location.z = banner_height - (jump_size / 2) + cos((bla / jump_duration) * 3.1415) * (jump_size / 2)

    if add_keyframes:
        camera.keyframe_insert(data_path="location", frame=scene.frame_current)
        camera.keyframe_insert(data_path="rotation_euler", frame=scene.frame_current)

    key(positions[i])
    render()
    scene.frame_current += 1
    i += 1

## ROTATION

rot = 0
rotspeed = 0

for [red, blue] in positions[jump_duration:]:

    print(scene.frame_current)

    new_center = (red['core'] + blue['core']) / 2

    camera_center.x = lerp(camera_center.x, new_center.x, 0.01)
    camera_center.z = lerp(camera_center.z, new_center.z, 0.01)
    camera_center.y = max(0.7, lerp(camera_center.y, new_center.y, 0.009))

    set_cam(camera_center, rot)

    if add_keyframes:
        camera.keyframe_insert(data_path="location", frame=scene.frame_current)
        camera.keyframe_insert(data_path="rotation_euler", frame=scene.frame_current)

    movelight(camera_center + lightoff)

    rot += rotspeed

    if rotspeed < 0.015: rotspeed += 0.000035

    #p = blue
    #cmu_enrich(p)

    #if add_keyframes:
    #    mark = p['rightfingers']
    #    marker = bpy.data.objects['marker']
    #    marker.location.x = mark.x
    #    marker.location.y = -mark.z
    #    marker.location.z = mark.y # + 1
    #    marker.keyframe_insert(data_path="location", frame=bpy.data.scenes[0].frame_current)

    if add_keyframes or (do_render and scene.frame_current >= render_start):
        key([red, blue])
        render()

    scene.frame_current += 1


scene.frame_current = old_frame

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
