# -*- coding: utf-8 -*-

import SCons.Defaults, SCons.Tool, SCons.Util, os

def add_glob(target, source, env):
  base, _ = os.path.splitext(str(target[0]))
  target.append(base + ".glob")
  return target, source

Coq = SCons.Builder.Builder(
  action = '$COQCMD',
  suffix = '.vo',
  src_suffix = '.v',
  emitter = add_glob)

def coqdoc_gen(source, target, env, for_signature):
  for s in source:
    base, _ = os.path.splitext(str(s))
    env.Depends(target, env.File(base + '.glob'))
  return [SCons.Defaults.Mkdir(target), 'coqdoc $COQDOCFLAGS -d $TARGET $COQFLAGS $SOURCES']

CoqDoc = SCons.Builder.Builder(generator = coqdoc_gen)

def generate(env):
  env['COQC'] = 'coqc'
  env['COQCMD'] = '$COQC $COQFLAGS -q $SOURCE'
  env.Append(BUILDERS = {'Coq': Coq, 'CoqDoc': CoqDoc})

def exists(env):
  return env.Detect('coqc')