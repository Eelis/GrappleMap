Require Import Reals Rtrigo1 Program.Basics Rsqrt_def Classes.Morphisms.
Require Import StdlibExtras Math.

Record Reo: Set := reo
  { offset: V3
  ; rotation: R }.

Definition apply (r: Reo): V3 -> V3 :=
  compose (V3translate (offset r)) (yrotate (rotation r)).

Definition composeReo (a b: Reo): Reo := reo
  (offset b <+> yrotate (rotation b) (offset a))
  (rotation a + rotation b).

Definition inverse (r: Reo): Reo :=
  reo (yrotate (- rotation r) (V3opp (offset r))) (- rotation r).


Definition compose_works v a b:
  apply (composeReo a b) v = apply b (apply a v).
Proof.
 intros.
 unfold apply, compose.
 simpl.
 rewrite yrotate_plus.
 rewrite yrotate_ding.
 apply V3e; simpl; ring.
Qed.

Lemma inverse_def (r: Reo):
  inverse r = reo (M3mult (yrot (- rotation r)) (V3opp (offset r))) (- rotation r).
Proof.
  reflexivity.
Qed.

Hint Unfold x3.

Lemma inv_works r v:
  apply (inverse r) (apply r v) = v.
Proof.
 unfold apply.
 simpl.
 destruct r as [o r].
 simpl.
 unfold yrotate, compose, V3translate.
 rewrite M3mult_V3opp.
 rewrite M3mult_V3plus.
 rewrite yrot_opp.
 set (M3mult (yrot (- r)) o).
 clearbody v0.
 destruct v0 as [aa bb cc].
 unfold V3plus, V3opp, x3, y3, z3.
 destruct v.
 apply V3e; simpl; ring.
Qed.

Lemma inv_involutive: forall v, inverse (inverse v) = v.
Proof with auto.
 intros.
 unfold inverse.
 simpl.
 rewrite Ropp_involutive.
 destruct v...
 f_equal.
 simpl.
 unfold yrotate.
 rewrite M3mult_V3opp.
 change (V3opp (yrotate rotation0 (yrotate (-rotation0) (V3opp offset0))) = offset0).
 rewrite <- yrotate_plus.
 fold (rotation0 - rotation0)%R.
 rewrite (Rminus_diag_eq)...
 rewrite yrotate_0.
 unfold idd.
 apply V3opp_involutive.
Qed.

Inductive CenterJoint: Set := Head | Core.

Variable (SideJoint: Set).

Notation Side := bool.

Definition Joint: Set := ((Side * SideJoint) + CenterJoint)%type.

Definition PlayerPos: Set := Joint -> V3.

Definition PlayerName := bool.

Definition Position: Set := PlayerName -> PlayerPos.

Definition swapLimbs' (p: PlayerPos): PlayerPos :=
  fun a => match a with
           | inl (s, j) => p (inl _ (negb s, j))
           | inr j => p (inr j)
           end.

Definition swap (p: Position): Position := fun x => p (negb x).

Definition mapP (f: PlayerPos -> PlayerPos) (p: Position): Position :=
  fun x => f (p x).

Definition swapLimbs: Position -> Position := mapP swapLimbs'.

Definition mapCoords (f: V3 -> V3): Position -> Position :=
  mapP (fun x j => f (x j)).



Definition mirror (p: Position): Position := mapCoords V3mirror (swapLimbs p).


Record PosReo: Set := posReo
  { pr_reo: Reo
  ; pr_mirror: bool
  ; pr_swap: bool
  }.

Definition app (r: Reo) (p: PlayerPos): PlayerPos :=
  fun x => apply r (p x).

Definition appl (r: Reo): Position -> Position := mapCoords (apply r).


Lemma swapLimbs_involutive p:
  swapLimbs (swapLimbs p) = p.
Proof with auto.
 intros.
 apply ext_eq.
 intros.
 apply ext_eq.
 intros [?|?]...
 destruct p0.
 simpl.
 rewrite Bool.negb_involutive...
Qed.



Definition swapIf (b: bool): Position -> Position :=
  if b then swap else idd.

Definition mirrorIf (b: bool): Position -> Position :=
  if b then mirror else idd.

Definition apply2 (r: PosReo): Position -> Position :=
  compose (  swapIf (pr_swap   r))
 (compose (mirrorIf (pr_mirror r))
          (    appl (pr_reo    r))).

Definition inverse2 (r: PosReo): PosReo :=
  posReo
    (if pr_mirror r
         then reo
           (V3opp (yrotate (rotation (pr_reo r)) (V3mirror (offset (pr_reo r)))))
           (rotation (pr_reo r))
         else inverse (pr_reo r))
    (pr_mirror r) (pr_swap r).

Lemma swap_mirror p:
  swap (mirror p) = mirror (swap p).
Proof.
  reflexivity.
Qed.

Lemma mapCoords_swapLimbs f p:
  mapCoords f (swapLimbs p) = swapLimbs (mapCoords f p).
Proof with auto.
 intros. apply ext_eq. intros.
 unfold mapCoords, swapLimbs.
 apply ext_eq.
 intros []...
 intros []...
Qed.

Theorem inverse2_works: forall r p, apply2 (inverse2 r) (apply2 r p) = p.
Proof with auto.
 intros.
 destruct r.
 destruct pr_reo0.
 unfold inverse2.
 simpl.
 unfold apply2.
 simpl.
 unfold compose.
 unfold mirrorIf.
 unfold swapIf.
 destruct pr_swap0.
   destruct pr_mirror0.
    unfold mirror.
    apply ext_eq.
    intros.
    apply ext_eq.
    intros.
    simpl.
    unfold swap.
    unfold appl.
    unfold compose.
    simpl.
    rewrite mapCoords_swapLimbs.
    unfold mapCoords.
    unfold mapP.
    unfold swapLimbs.
    unfold mapP.
    rewrite Bool.negb_involutive.
    unfold swapLimbs'.
    unfold apply.
    simpl.
    unfold compose.
    destruct x0.
     destruct p0.
     rewrite V3mirror_V3plus.
     simpl.
     rewrite Bool.negb_involutive.
     rewrite V3mirror_V3plus.
     rewrite yrotate_ding.
     rewrite V3mirror_V3plus.
     rewrite V3mirror_yrotate.
     rewrite <- V3opp_V3mirror.
     rewrite V3plus_assoc.
     simpl.
     set (yrotate rotation0 (V3mirror offset0)).
     rewrite (V3plus_comm (V3opp (V3mirror v))).
     rewrite V3plus_V3opp.
     apply V3e; simpl; ring.
    set (p x (inr c)).
    repeat rewrite V3mirror_V3plus.
    rewrite yrotate_ding.
    rewrite V3mirror_V3plus.
    rewrite V3mirror_yrotate.
    apply V3e; simpl; ring.
   unfold idd.
   unfold appl.
   unfold mapCoords.
   unfold mapP.
   apply ext_eq.
   intro. apply ext_eq.
   intros.
   unfold swap.
   rewrite Bool.negb_involutive.
   apply inv_works.
  unfold idd.
  unfold appl.
  destruct pr_mirror0.
   unfold mirror.
   repeat rewrite mapCoords_swapLimbs.
   rewrite swapLimbs_involutive.
   apply ext_eq.
   intro.
   unfold mapCoords, mapP, apply.
   simpl.
   apply ext_eq.
   intro.
   unfold compose.
   do 2 rewrite V3mirror_V3plus.
   rewrite yrotate_ding.
   rewrite V3mirror_V3plus.
   rewrite V3mirror_yrotate.
   set (p x x0).
   apply V3e; simpl; ring.
  apply ext_eq.
  intros ?.
  apply ext_eq.
  intros ?.
  unfold mapCoords.
  unfold mapP.
  apply inv_works.
Qed.

Definition P1core (p: Position): V3 := p true (inr _ Core).
Definition P0core (p: Position): V3 := p false (inr _ Core).



Definition posCenter (p: Position): V3 := V3mult (P0core p <+> P1core p) (1/2)%R.



(* Normalization *)

Definition translationReo (v: V3): Reo :=
  reo v 0.

Definition normalTranslationV3 (p: Position): V3 := V3opp (posCenter p).

Definition normalTranslation (p: Position): Reo :=
  translationReo (normalTranslationV3 p).

Definition normalTranslate (p: Position): Position :=
  appl (normalTranslation p) p.

Definition heading (p: Position): V2
  := xz (P1core p <+> V3opp (P0core p)).

Definition normalRotation (p: Position): R
  := (- atan2 (heading p))%R.

Definition normalRotationReo (p: Position): Reo :=
  reo V3zero (normalRotation p). (* todo: use compose *)

Definition Protate (a: R): Position -> Position
  := mapCoords (yrotate a).

Definition Ptranslate (v: V3): Position -> Position
  := mapCoords (V3translate v).

Definition normalRotate (p: Position): Position
  := Protate (normalRotation p) p.

Definition ValidPos (p: Position): Prop
  := xz (P0core p) <> xz (P1core p).

Lemma normalRotate_Protate a p:
  ValidPos p ->
  normalRotate (Protate a p) = normalRotate p.
Proof with auto.
 intros.
 apply ext_eq.
 intros.
 apply ext_eq.
 intros.
 unfold normalRotate.
 unfold Protate.
 unfold mapCoords.
 unfold mapP.
 fold (P0core p) in *.
 fold (P1core p) in *.
 set (v := p _ _).
 rewrite <- yrotate_plus.
 apply yrotate_mod.
 unfold normalRotation.
 unfold heading.
 unfold P0core.
 unfold P1core.
 rewrite <- yrotate_V3opp.
 rewrite <- yrotate_ding.
 set (fcore := p false (inr Core)).
 rewrite rotate_xz.
 fold P0core.
 set (tcore := p true (inr Core)) in *.
 set (c := tcore <+> V3opp fcore).
 replace (- atan2 (rotate a (xz c)) + a)%R with (- (atan2 (rotate a (xz c)) - a))%R by ring.
 apply sameAngle_inv.
 apply sameAngle_eq_reg_r with a.
 replace (atan2 (rotate a (xz c)) -a + a)%R with (atan2 (rotate a (xz c))) by ring.
 apply atan2_rotate...
 unfold c.
 unfold V3opp.
 simpl.
 unfold xz.
 simpl.
 intro.
 pose proof (length_0 _ H0).
 apply H.
 inversion H1.
 clear H1.
 unfold xz.
 rewrite H3 in H4.
 subst fcore tcore.
 fold (P0core p) (P1core p) in *.
 f_equal.
  rewrite <- (Rplus_0_r (x3 (P0core p))).
  rewrite <- H3.
  ring.
 rewrite <- (Rplus_0_r (z3 (P0core p))).
 rewrite <- H4.
 ring.
Qed.

Lemma heading_Protate r p:
  heading (Protate r p) = rotate r (heading p).
Proof.
 unfold heading, Protate, P0core, P1core, mapCoords, mapP, rotate, xz.
 simpl. f_equal; ring.
Qed.

Lemma ValidPos_heading p:
  ValidPos p ->
  heading p <> V2zero.
Proof with auto with arith real.
 intros.
 unfold heading.
 unfold V3plus.
 unfold xz.
 simpl.
 intro.
 inversion H0.
 clear H0.
 apply H.
 unfold xz.
 f_equal.
  rewrite <- (Ropp_involutive (x3 (P0core p))).
  rewrite (Rplus_opp_r_uniq _ _ H2)...
 rewrite H2 in H3.
 rewrite <- (Ropp_involutive (z3 (P0core p))).
 rewrite (Rplus_opp_r_uniq _ _ H3)...
Qed.

Lemma normalRotation_Protate r p:
  ValidPos p ->
  sameAngle (normalRotation (Protate r p)) (normalRotation p - r).
Proof with auto.
 unfold normalRotation.
 rewrite heading_Protate.
 replace (- atan2 (heading p) - r)%R with (- (atan2 (heading p) + r))%R by ring.
 intros.
 apply sameAngle_inv.
 apply atan2_rotate...
 intro.
 apply (ValidPos_heading _ H).
 apply length_0...
Qed.

Lemma normalRotation_normalRotate p:
  ValidPos p ->
  sameAngle (normalRotation (normalRotate p)) 0%R.
Proof with auto.
 unfold normalRotate.
 intros.
 apply sameAngle_trans with (normalRotation p - normalRotation p)%R.
  apply normalRotation_Protate...
 unfold Rminus.
 rewrite (Rplus_opp_r (normalRotation p)).
 apply sameAngle_refl.
Qed.



Definition normal (p: Position): Reo :=
  reo (normalTranslationV3 (normalRotate p)) (normalRotation p).

Definition normalizeRT (p: Position): Position :=
  appl (normal p) p.


Lemma mapCoords_comp f g p:
  mapCoords f (mapCoords g p) = mapCoords (compose f g) p.
Proof.
 reflexivity.
Qed.

Lemma posCenter_normalizeRT p:
  xz (posCenter (normalizeRT p)) = v2 0 0.
Proof with auto.
 unfold normalizeRT.
 unfold appl.
 unfold normal.
 unfold apply.
 simpl offset.
 simpl rotation.
 unfold normalTranslationV3.
 rewrite <- mapCoords_comp.
 unfold normalRotate.
 unfold Protate.
 set (mapCoords (yrotate (normalRotation p)) p).
 clearbody p0.
 unfold xz.
 f_equal.
  simpl.
  fold (P0core p0).
  fold (P1core p0).
  unfold pow.
  field.
 simpl.
 unfold P0core, P1core.
 field.
Qed.

Lemma normalRotation_translationReo v p:
  normalRotation (appl (translationReo v) p) = normalRotation p.
Proof with auto.
 unfold normalRotation, appl, mapCoords, mapP, heading.
 unfold P0core, P1core, normalTranslation, apply, compose.
 simpl rotation.
 rewrite yrotate_0.
 f_equal.
 f_equal.
 unfold idd.
 f_equal.
 apply V3e; simpl; ring.
Qed.

Lemma normalRotation_Ptranslate v p:
  normalRotation (Ptranslate v p) = normalRotation p.
Proof with auto.
 intros.
 pose proof (normalRotation_translationReo v p).
 unfold appl in H.
 unfold apply in H.
 simpl rotation in H.
 rewrite yrotate_0 in H...
Qed.

Lemma compose_works_appl a b p:
  appl (composeReo a b) p = appl b (appl a p).
Proof.
 apply ext_eq.
 intro.
 apply ext_eq.
 intro.
 unfold appl.
 unfold mapCoords.
 unfold mapP.
 rewrite compose_works.
 reflexivity.
Qed.

Lemma normalizeRT_translationReo v a:
  normalizeRT (appl (translationReo v) a) = normalizeRT a.
Proof with auto.
 intros.
 unfold normalizeRT.
 unfold normal at 1.
 unfold normalRotate.
 rewrite normalRotation_translationReo.
 rewrite <- (compose_works_appl).
 f_equal.
 unfold composeReo.
 simpl rotation.
 simpl offset.
 rewrite Rplus_0_l.
 unfold normal.
 f_equal.
 unfold normalTranslationV3.
 unfold appl.
 unfold apply.
 simpl rotation.
 rewrite yrotate_0.
 rewrite compose_idd_r.
 simpl offset.
 unfold posCenter.
 apply V3e; simpl; field.
Qed.

Lemma normalizeRT_Protate p r:
  ValidPos p ->
  normalizeRT (Protate r p) = normalizeRT p.
Proof with auto.
 unfold normalizeRT.
 unfold normal.
 intros.
 unfold appl.
 unfold apply.
 intros.
 simpl.
 apply ext_eq.
 intro.
 apply ext_eq.
 intro.
 unfold mapCoords.
 unfold mapP.
 unfold compose.
 rewrite normalRotate_Protate...
 unfold normalRotate.
 unfold V3translate.
 rewrite normalRotation_Protate...
 unfold Protate at 2.
 unfold mapCoords, mapP.
 rewrite <- yrotate_plus.
 replace (normalRotation p - r + r)%R with (normalRotation p) by ring...
Qed. 

Theorem normalizeRT_appl p r:
  ValidPos p ->
  normalizeRT (appl r p) = normalizeRT p.
Proof with auto.
 intros.
 unfold appl.
 unfold apply.
 rewrite <- mapCoords_comp.
 pose proof (normalizeRT_translationReo (offset r) (mapCoords (yrotate (rotation r)) p)).
 unfold appl in H0.
 unfold apply in H0.
 simpl rotation in H0.
 rewrite yrotate_0 in H0.
 rewrite compose_idd_r in H0.
 simpl offset in H0.
 rewrite H0.
 apply normalizeRT_Protate...
Qed.

Remark normalizeRT_involutive p:
  ValidPos p ->
  normalizeRT (normalizeRT p) = normalizeRT p.
Proof.
 intros.
 apply (normalizeRT_appl p).
 assumption.
Qed.



(* normalization /including/ mirror: *)

Definition needsMirror (p: Position): bool :=
  if Rle_lt_dec (x3 (p false (inr _ Head))) 0 then true else false.

Definition mirrorReoNormalizer (p: Position): PosReo := posReo
  (normal p)
  (needsMirror (appl (normal p) p))
  false (* swap *).

Definition mirrorReoNormalize (p: Position): Position :=
  apply2 (mirrorReoNormalizer p) p.


Lemma normal_swapLimbs a: normal (swapLimbs a) = normal a.
Proof.
 reflexivity.
Qed.

Lemma normalRotation_swapLimbs a:
  normalRotation (swapLimbs a) = normalRotation a.
Proof.
 reflexivity.
Qed.

Lemma normalRotation_V3mirror a:
  ValidPos a ->
  sameAngle
    (normalRotation (mapCoords V3mirror a))
    (- normalRotation a)%R.
Proof with auto.
 unfold normalRotation.
 unfold heading.
 rewrite Ropp_involutive.
 unfold P0core, P1core.
 unfold mapCoords, mapP.
 fold (P0core a) (P1core a).
 rewrite V3opp_V3mirror.
 rewrite <- V3mirror_V3plus.
 intros.
 assert (xz (P1core a <+> V3opp (P0core a)) <> v2 0 0)%R.
  unfold ValidPos in H.
  intro.
  unfold xz in H0.
  inversion H0.
  apply H.
  unfold xz.
  destruct (P0core a) in *.
  destruct (P1core a) in *.
  simpl in *.
  f_equal.
   rewrite <- (Ropp_involutive x0).
   rewrite <- (Ropp_involutive x3).
   rewrite (Rplus_opp_r_uniq _ _ H2)...
  rewrite H2 in H3.
  rewrite <- (Ropp_involutive z0).
  rewrite <- (Ropp_involutive z3).
  rewrite (Rplus_opp_r_uniq _ _ H3)...
 revert H0.
 clear H.
 generalize (P1core a <+> V3opp (P0core a)).
 intros.
 replace (xz (V3mirror v)) with (V2mirror (xz v))...
 rewrite <- (Ropp_involutive (atan2 (xz v))).
 apply sameAngle_inv.
 apply atan2_mirror...
Qed.

Lemma normalTranslation_swapLimbs p:
  normalTranslationV3 (swapLimbs p) = normalTranslationV3 p.
Proof.
 reflexivity.
Qed.

Lemma normalRotate_swapLimbs p:
  normalRotate (swapLimbs p) = swapLimbs (normalRotate p).
Proof.
 unfold normalRotate, Protate.
 rewrite normalRotation_swapLimbs, mapCoords_swapLimbs.
 reflexivity.
Qed.

Lemma normal_mirror_rot a:
  ValidPos a ->
  sameAngle (rotation (normal (mirror a))) (- rotation (normal a))%R.
Proof.
 simpl.
 unfold mirror.
 rewrite mapCoords_swapLimbs.
 rewrite normalRotation_swapLimbs.
 apply normalRotation_V3mirror.
Qed.

Lemma normal_mirror a:
  ValidPos a ->
  offset (normal (mirror a)) = V3mirror (offset (normal a)).
Proof with auto.
 intros.
 unfold mirror.
 rewrite mapCoords_swapLimbs.
 unfold normal.
 simpl.
 rewrite normalRotate_swapLimbs.
 unfold normalRotate, Protate.
 rewrite normalTranslation_swapLimbs.
 rewrite (normalRotation_V3mirror a)...
 rewrite mapCoords_comp.
 unfold compose.
 unfold normalTranslationV3.
 unfold posCenter.
 unfold P0core.
 unfold P1core.
 unfold mapCoords.
 unfold mapP.
 fold (P0core a) (P1core a).
 do 2 rewrite yrotate_Ropp.
 rewrite <- V3mirror_V3plus.
 rewrite V3mult_V3mirror.
 do 2 rewrite yrotate_V3mirror.
 rewrite <- V3opp_V3mirror.
 f_equal.
 f_equal.
 do 2 rewrite V3mirror_involutive.
 rewrite Ropp_involutive...
Qed.

Lemma normalize_mirror a:
  ValidPos a ->
  normalizeRT (mirror a) = mirror (normalizeRT a).
Proof with auto.
 unfold normalizeRT.
 pose proof (normal_mirror a).
 pose proof (normal_mirror_rot a).
 set (normal (mirror a)) in *.
 clearbody r.
 intros.
 apply ext_eq. intro.
 apply ext_eq. intro.
 unfold appl.
 unfold mirror.
 rewrite mapCoords_comp.
 rewrite mapCoords_swapLimbs.
 rewrite mapCoords_swapLimbs.
 f_equal.
 unfold mapCoords, mapP.
 apply ext_eq. intro.
 apply ext_eq. intro.
 unfold compose.
 set (normal a) in *.
 clearbody r0.
 unfold apply.
 unfold compose.
 unfold V3translate.
 rewrite V3mirror_V3plus.
 rewrite <- H...
 rewrite yrotate_V3mirror.
 rewrite H0...
 rewrite Ropp_involutive...
Qed.

Lemma needsMirror_mirror p:
  x3 (p false (inr Head)) <> 0%R ->
  needsMirror (mirror p) = negb (needsMirror p).
Proof with auto.
 unfold mirror, mapCoords, needsMirror, mapP, V3mirror.
 intros.
 simpl.
 destruct Rle_lt_dec.
  destruct Rle_lt_dec...
  simpl.
  exfalso.
  apply H.
  apply Rle_antisym...
  apply Ropp_le_cancel...
  rewrite Ropp_0...
 destruct Rle_lt_dec...
 rewrite <- Ropp_0 in r.
 pose proof (Ropp_lt_cancel _ _ r).
 exfalso.
 apply (Rlt_asym _ _ r0)...
Qed.

Lemma mirror_involutive p:
  mirror (mirror p) = p.
Proof.
 do 2 (apply ext_eq; intro).
 unfold mirror.
 do 3 rewrite mapCoords_swapLimbs.
 rewrite swapLimbs_involutive.
 apply V3mirror_involutive.
Qed.

Instance polar_proper: Proper (sameAngle ==> eq ==> eq) polar.
Proof with auto.
 repeat intro.
 destruct H.
 subst.
 unfold polar, V2mult.
 simpl.
 f_equal.
  f_equal. rewrite <- (sin_period_Z x) with x1. f_equal. ring.
 f_equal. rewrite <- (cos_period_Z x) with x1. f_equal. ring.
Qed.


Lemma polar_eq3 a b c d:
  polar a b = polar c d ->
  sameAngle a c ->
  b = d.
Proof with auto.
 unfold polar, V2mult.
 simpl.
 intros.
 destruct H0.
 subst.
 replace (a + IZR x * (2 * PI))%R with (a + 2 * IZR x * PI)%R in * by ring.
 rewrite sin_period_Z in H.
 rewrite cos_period_Z in H.
 inversion H.
 destruct (cos_sin_0_var a).
  apply Rmult_eq_reg_l with (cos a)...
 apply Rmult_eq_reg_l with (sin a)...
Qed.

Lemma polar_eq4 a b c d:
  polar a b = polar c d ->
  Rabs b = Rabs d.
Proof with auto.
 intros.
 assert (length (polar a b) = length (polar c d)).
  rewrite H...
 do 2 rewrite length_polar in H0...
Qed.

Lemma length_nonneg v: (0 <= length v)%R.
Proof.
 apply sqrt_pos.
Qed.

Lemma rotate_eq_reg_r v w r:
  rotate r v = rotate r w ->
  v = w.
Proof with auto.
 intros.
 assert (length w = length v).
  rewrite <- (length_rotate r v).
  rewrite <- (length_rotate r w).
  rewrite H...
 destruct (Req_dec (length v) 0).
  rewrite H1 in H0.
  pose proof (length_0 _ H1).
  pose proof (length_0 _ H0).
  subst...
 revert H H0 H1.
 rewrite <- (atan2_spec v).
 rewrite <- (atan2_spec w).
 cut (0 <= length v)%R.
  cut (0 <= length w)%R.
   generalize (atan2 v) (length v) (atan2 w) (length w).
   intros ????.
   do 2 rewrite rotate_polar.
   intros.
   pose proof (polar_eq4 _ _ _ _ H1).
   do 2 rewrite Rabs_pos_eq in H4...
   subst.
   rewrite length_polar in H3.
   rewrite Rabs_pos_eq in H3...
   pose proof (polar_eq _ _ _ H3 H1).
   apply polar_proper...
   apply sameAngle_eq_reg_r with r.
   rewrite (Rplus_comm r0), (Rplus_comm r2)...
  apply length_nonneg.
 apply length_nonneg.
Qed.

Lemma xz_rotate r v:
  xz (yrotate r v) = rotate r (xz v).
Proof with auto.
 unfold yrotate, yrot, rotate, M3mult, xz.
 simpl.
 f_equal; ring.
Qed.

Lemma V2plus_eq_reg_l a b c:
  V2plus a b = V2plus a c -> b = c.
Proof with auto.
 destruct b as [bx by'], c as [cx cy].
 unfold V2plus.
 simpl.
 intros.
 inversion H.
 f_equal.
  apply Rplus_eq_reg_l with (x2 a)...
 apply Rplus_eq_reg_l with (y2 a)...
Qed.

Lemma ValidPos_appl r p:
  ValidPos p ->
  ValidPos (appl r p).
Proof with auto.
 intros.
 change (xz (offset r <+> yrotate (rotation r) (P0core p)) <>
         xz (offset r <+> yrotate (rotation r) (P1core p))).
 do 2 rewrite xz_V3plus.
 do 2 rewrite xz_rotate.
 intro.
 apply H.
 apply rotate_eq_reg_r with (rotation r).
 apply V2plus_eq_reg_l with (xz (offset r))...
Qed.

Theorem mirrorReoNormalize_apply2:
  forall a, ValidPos a ->
    x3 (normalizeRT a false (inr Head)) <> 0%R -> (* i think i know how to get rid of this condition *)
  forall r, pr_swap r = false ->
    mirrorReoNormalize (apply2 r a) = mirrorReoNormalize a.
Proof with auto.
 intros.
 unfold mirrorReoNormalize.
 destruct r.
 simpl in H1.
 subst.
 unfold apply2.
 simpl.
 repeat rewrite compose_idd_l.
 unfold compose.
 destruct pr_mirror0; simpl.
  unfold compose.
  fold (normalizeRT (mirror (appl pr_reo0 a))).
  fold (normalizeRT a).
  rewrite normalize_mirror.
   rewrite normalizeRT_appl...
   rewrite (needsMirror_mirror (normalizeRT a))...
   generalize (normalizeRT a).
   intros.
   destruct (needsMirror p)...
   unfold mirrorIf.
   simpl.
   apply mirror_involutive.
  apply ValidPos_appl...
 unfold idd.
 fold (normalizeRT (appl pr_reo0 a)).
 rewrite normalizeRT_appl...
Qed.


Print Assumptions mirrorReoNormalize_apply2.
