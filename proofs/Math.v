Require Import Reals Rtrigo1 Program.Basics Rsqrt_def Classes.Morphisms.
Require Import StdlibExtras.

Record V2 := v2
  { x2: R
  ; y2: R }.

Record V3 := v3
  { x3: R
  ; y3: R
  ; z3: R }.

Record M3: Set := m3
  { m00: R; m01: R; m02: R
  ; m10: R; m11: R; m12: R
  ; m20: R; m21: R; m22: R }.

(* Operations *)

Definition V2plus (a b: V2): V2 := v2 (x2 a + x2 b) (y2 a + y2 b).
Definition V3plus (a b: V3): V3 := v3 (x3 a + x3 b) (y3 a + y3 b) (z3 a + z3 b).

Definition V2mult (v: V2) (s: R): V2 := v2 (x2 v * s) (y2 v * s).
Definition V3mult (v: V3) (r: R): V3 := v3 (x3 v * r) (y3 v * r) (z3 v * r).

Definition V2opp (v: V2): V2 := v2 (- x2 v) (- y2 v).
Definition V3opp (v: V3): V3 := v3 (- x3 v) (- y3 v) (- z3 v).

Definition M3mult (m: M3) (v: V3): V3 := v3
  ( m00 m * x3 v + m10 m * y3 v + m20 m * z3 v )
  ( m01 m * x3 v + m11 m * y3 v + m21 m * z3 v )
  ( m02 m * x3 v + m12 m * y3 v + m22 m * z3 v ).

Definition V2mirror (v: V2): V2 := v2 (- x2 v) (y2 v).
Definition V3mirror (v: V3): V3 := v3 (- x3 v) (y3 v) (z3 v).

Definition V2zero: V2 := v2 0 0.
Definition V3zero: V3 := v3 0 0 0.

Definition Rdec_relation (a b: R): (a < b)%R + (b < a)%R + (a = b)%R.
Proof with auto.
 destruct (Rle_lt_dec a b)...
 destruct (Rle_lt_or_eq_dec a b)...
Qed.

Definition atan2 (v: V2): R.
 destruct v as [x y].
 destruct (Rdec_relation x 0) as [[?|?]|?]; destruct (Rdec_relation y 0) as [[?|?]|?].
         exact (atan (y / x) - PI)%R.
        exact (atan (y / x) + PI)%R.
       exact (atan (y / x) + PI)%R.
      exact (atan (y / x))%R.
     exact (atan (y / x))%R.
    exact (atan (y / x))%R.
   exact (- PI / 2)%R.
  exact (PI / 2)%R.
 exact 0%R.
Defined.
    
Definition length (v: V2): R := sqrt (Rsqr (x2 v) + Rsqr (y2 v))%R.

Definition xz (v: V3): V2 := v2 (x3 v) (z3 v).

Notation "a <+> b" := (V3plus a b) (at level 80, right associativity).



Definition polar (a n: R): V2 := V2mult (v2 (sin a) (cos a)) n.





Definition yrot (a: R): M3 := m3
  (cos a) 0 (- sin a)
  0 1 0
  (sin a) 0 (cos a).

Definition y0 (v: V2): V3 := v3 (x2 v) 0 (y2 v).

Definition yrotate (a: R): V3 -> V3 :=
  M3mult (yrot a).

Definition rotate r (v: V2): V2 :=
  v2
   (cos r * x2 v + sin r * y2 v)
   (- sin r * x2 v + cos r * y2 v).

Lemma rotate_xz r v: xz (yrotate r v) = rotate r (xz v).
Proof.
 unfold xz, rotate, yrotate, mult, yrot.
 simpl. f_equal; ring.
Qed.

Definition V3translate: V3 -> V3 -> V3 := V3plus.




(* Lemmas *)

Lemma V3e: forall a b, x3 a = x3 b -> y3 a = y3 b -> z3 a = z3 b -> a = b.
Proof.
 intros [???] [???].
 unfold x3, y3, z3. simpl.
 congruence.
Qed.

Lemma V3plus_comm x y: (x <+> y) = (y <+> x).
Proof.
 apply V3e; simpl; ring.
Qed.

Lemma yrotate_0: yrotate 0 = idd.
Proof.
 apply ext_eq.
 intro.
 unfold yrotate, yrot, mult, idd.
 simpl.
 rewrite sin_0, cos_0.
 apply V3e; simpl; ring.
Qed.

Lemma xz_V3plus a b:
  xz (a <+> b) = V2plus (xz a) (xz b).
Proof.
 reflexivity.
Qed.

Lemma V3plus_assoc (a b c: V3): (a <+> (b <+> c)) = ((a <+> b) <+> c).
Proof.
  apply V3e; repeat intro; simpl; ring.
Qed.

Lemma x_V3plus (a b: V3): x3 (V3plus a b) = (x3 a + x3 b)%R.
Proof.
 reflexivity.
Qed.

Definition yrotate_ding a (v w: V3):
  yrotate a (V3plus v w) = V3plus (yrotate a v) (yrotate a w).
Proof.
 apply V3e; simpl; ring.
Qed.

Definition yrotate_plus (a b: R) (v: V3):
  yrotate (a + b) v = yrotate a (yrotate b v).
Proof.
 apply V3e; simpl; try rewrite cos_plus, sin_plus; ring.
Qed.

Lemma V3opp_involutive v:
  V3opp (V3opp v) = v.
Proof.
 apply V3e; simpl; intros; ring.
Qed.

Lemma M3mult_V3opp m v:
  M3mult m (V3opp v) = V3opp (M3mult m v).
Proof.
 apply V3e; unfold x3, y3, z3; simpl; ring.
Qed.

Lemma yrotate_V3opp v r: yrotate r (V3opp v) = V3opp (yrotate r v).
Proof.
 apply V3e; intros; simpl; ring.
Qed.

Lemma M3mult_V3plus m v w: M3mult m (V3plus v w) = V3plus (M3mult m v) (M3mult m w).
Proof.
 apply V3e; unfold M3mult, V3plus, x3, y3, z3; simpl; ring.
Qed.

Lemma z_mult_yrot a v:
  z3 (M3mult (yrot a) v) = (cos a * z3 v - sin a * x3 v)%R.
Proof.
 unfold M3mult, yrot, z3. simpl. ring_simplify. auto.
Qed.

Lemma x_mult_yrot: forall a v, x3 (M3mult (yrot a) v) = (cos a * x3 v + sin a * z3 v)%R.
Proof.
 unfold M3mult, yrot, x3.
 simpl.
 intros.
 ring_simplify.
 reflexivity.
Qed.

Lemma yrot_opp: forall r v, M3mult (yrot (- r)) (M3mult (yrot r) v) = v. (* todo: rephrase *)
Proof.
 intros.
 change (yrotate (-r) (yrotate r v) = v).
 rewrite <- yrotate_plus.
 rewrite Rplus_opp_l.
 rewrite yrotate_0; auto.
Qed.

Lemma length_mult: forall v s, length (V2mult v s) = (length v * Rabs s)%R.
Proof with auto with real arith.
 intros.
 unfold V2mult, length.
 destruct v.
 simpl.
 replace (Rabs s) with (sqrt (Rsqr (Rabs s))).
  rewrite <- sqrt_mult...
   f_equal.
   rewrite <- Rsqr_abs.
   unfold Rsqr.
   ring.
  apply Rplus_le_le_0_compat...
 apply sqrt_Rsqr...
 apply Rabs_pos.
Qed.

Lemma length_unit a: length {| x2 := sin a; y2 := cos a |} = 1%R.
Proof with auto.
  unfold length.
  simpl.
  generalize (Rplus_le_le_0_compat (sin a)² (cos a)² (Rle_0_sqr (sin a))
                   (Rle_0_sqr (cos a))).
  rewrite sin2_cos2.
  intros.
  apply sqrt_1.
Qed.

Lemma length_polar a s: length (polar a s) = Rabs s.
Proof with auto with real arith.
 unfold polar.
 rewrite length_mult, length_unit...
Qed.


Definition Req_mod (d: R) (x y: R): Prop :=
  exists n:Z, (x + IZR n * d = y)%R.


Definition sameAngle (a b: R): Prop := Req_mod (2 * PI) a b.

Instance cos_sameangle_proper: Proper (sameAngle ==> eq) cos.
Proof with try ring.
 intros ??[??].
 rewrite <- H.
 replace (x + IZR x0 * (2 * PI))%R with (x + 2 * IZR x0 * PI)%R...
 rewrite (cos_period_Z x x0)...
Qed.

Instance sin_sameangle_proper: Proper (sameAngle ==> eq) sin.
Proof.
 intros ??[??].
 rewrite <- H.
 rewrite <- (sin_period_Z x x0).
 f_equal.
 ring.
Qed.

Lemma sameAngle_both_low: forall a b,
 (0 <= a -> a <= PI ->
  0 <= b -> b <= PI ->
  cos a = cos b ->
  a = b)%R.
Proof with auto with real arith.
 intros.
 apply Rle_antisym.
  apply cos_decr_0...
 apply cos_decr_0...
Qed.

Lemma sameAngle_both_high: forall a b,
 (PI <= a -> a <= 2 * PI ->
  PI <= b -> b <= 2 * PI ->
  cos a = cos b ->
  a = b)%R.
Proof.
 intros.
 apply Rle_antisym; apply cos_incr_0; auto with real arith.
Qed.

Lemma sameAngle_trans a b c: sameAngle a b -> sameAngle b c -> sameAngle a c.
Proof.
 intros [p []] [q []].
 exists (p + q)%Z.
 rewrite plus_IZR.
 ring.
Qed.

Lemma sameAngle_refl a: sameAngle a a.
Proof.
 exists 0%Z. simpl. ring.
Qed.

Lemma sameAngle_sym a b: sameAngle a b -> sameAngle b a.
Proof with auto.
 intros [??].
 exists (- x)%Z.
 subst.
 rewrite opp_IZR.
 ring.
Qed.

Lemma sameAngle_by_sincos: forall a b,
  sin a = sin b ->
  cos a = cos b ->
    sameAngle a b.
Proof with auto with real.
 intros.
 destruct (euclidian_division a (2 * PI)).
  apply twopi_ne_0.
 destruct H1.
 destruct H1.
 subst.
 replace (IZR x * (2 * PI) + x0)%R with (x0 + 2 * IZR x * PI)%R in * by ring.
 rewrite (cos_period_Z) in H0.
 rewrite sin_period_Z in H.
 apply sameAngle_trans with x0.
  exists (- x)%Z.
  rewrite opp_IZR.
  ring.
 destruct (euclidian_division b (2 * PI)).
  apply twopi_ne_0.
 destruct H1. destruct H1.
 subst.
 apply sameAngle_trans with x4.
  replace (IZR x1 * (2 * PI) + x4)%R with (x4 + 2 * IZR x1 * PI)%R in * by ring.
  rewrite cos_period_Z in H0.
  rewrite sin_period_Z in H.
  destruct H2.
  destruct H3.
  rename x0 into a.
  rename x4 into b.
  clear x. clear x1.
  destruct (Rlt_le_dec a PI); destruct (Rlt_le_dec b PI).
     simpl.
     rewrite (sameAngle_both_low a b)...
     apply sameAngle_refl.
    destruct (sameAngle_low_high a b H1 r r0)...
     rewrite Rabs_pos_eq in H4...
     apply twopi_nonneg.
    subst.
    rewrite cos_PI in H0.
    rewrite cos_0 in H0.
    exfalso.
    apply (Ropp_ne 1)...
   exfalso.
   assert (b <= PI)%R.
    apply Rlt_le...
   assert (0 <= sin b)%R.
    apply sin_ge_0...
   assert (sin a <= 0)%R.
    apply sin_le_0...
    rewrite Rabs_pos_eq in H2...
    apply twopi_nonneg.
   assert (sin a = 0)%R.
    apply Rle_antisym...
    rewrite H...
   pose twopi_nonneg.
   rewrite Rabs_pos_eq in H2, H4...
   destruct (sin_eq_O_2PI_0 a)...
    subst.
    apply (Rlt_not_le PI 0)...
    apply pi_pos.
   destruct H9.
    subst.
    rewrite sin_PI in H.
    rewrite cos_PI in H0.
    clear H8 H7 r H2 H1 r1.
    destruct (sin_eq_O_2PI_0 b)...
     subst.
     rewrite cos_0 in H0.
     apply (Ropp_ne 1)...
    destruct H1.
     subst.
     apply (Rlt_irrefl PI)...
    subst.
    apply (Rlt_irrefl _ H4)...
   subst.
   apply (Rlt_irrefl _ H2).
  rewrite (sameAngle_both_high a b)...
    apply sameAngle_refl.
   rewrite Rabs_pos_eq in H2...
   apply twopi_nonneg.
  rewrite Rabs_pos_eq in H4...
  apply twopi_nonneg.
 exists x1.
 ring.
Qed.

Lemma v2_nonzero v: v <> V2zero <-> (x2 v <> 0 \/ y2 v <> 0)%R.
Proof with auto.
 intros.
 split.
  repeat intro.
  destruct (Req_dec (x2 v) 0)...
  destruct (Req_dec (y2 v) 0)...
  exfalso.
  apply H.
  destruct v.
  simpl in *.
  subst...
 repeat intro.
 subst.
 destruct H; apply H...
Qed.

Lemma positive_length v: v <> V2zero -> (0 < length v)%R.
Proof with auto with real arith.
 intros.
 unfold length.
 apply sqrt_lt_R0.
 rewrite v2_nonzero in H.
 destruct H.
  apply Rplus_lt_le_0_compat...
 apply Rplus_le_lt_0_compat...
Qed.

Lemma V2opp_polar a s: V2opp (polar a s) = polar a (- s)%R.
Proof.
 unfold polar, V2opp, V2mult.
 simpl. f_equal; ring.
Qed.


Definition Aop: R -> R := Rplus PI.

Lemma polar_Aop a s:
  polar (Aop a) s = polar a (- s).
Proof.
 unfold polar, V2mult, Aop. simpl.
 replace (PI + a)%R with (a + PI)%R by ring.
 rewrite neg_sin, neg_cos.
 f_equal; ring.
Qed. 

Lemma polar_0 r: polar r 0 = V2zero.
Proof.
 unfold polar, V2mult, V2zero. simpl.
 f_equal; ring.
Qed.

Lemma sin_x_minus_pi x: sin (x - PI) = (- sin x)%R.
Proof with auto.
 intros.
 rewrite <- (sin_period (x - PI) 1).
 replace (x - PI + 2 * INR 1 * PI)%R with (x + PI)%R by (simpl; ring).
 apply neg_sin.
Qed.

Lemma cos_x_minus_pi x: cos (x - PI) = (- cos x)%R.
Proof.
 rewrite <- (cos_period (x - PI) 1).
 replace (x - PI + 2 * INR 1 * PI)%R with (x + PI)%R by (simpl; ring).
 apply neg_cos.
Qed.

Lemma atan2_spec: forall v, polar (atan2 v) (length v) = v.
Proof with auto with real arith.
 unfold atan2.
 intros.
 destruct v as [x y].
 unfold length.
 simpl.
 destruct Rdec_relation as [[?|?]|?]; destruct Rdec_relation as [[?|?]|?]; try subst.
         (**)
         unfold polar.
         rewrite sin_x_minus_pi.
         rewrite cos_x_minus_pi.
         unfold V2mult.
         f_equal.
          simpl.
          admit.
         admit.
        admit.
       admit.
      admit.
     (* 0 < x /\ 0 < y *)
     unfold polar, V2mult.
     f_equal.
      simpl.
      admit.
     admit.
    admit.
   admit.
  admit.
 unfold Rsqr.
 rewrite Rmult_0_r, Rplus_0_r, sqrt_0, polar_0...
Admitted. (* dammit, my trig-fu is not strong enough! *)

Lemma atan2_polar r s:
  (0 < s)%R ->
  sameAngle (atan2 (polar r s)) r.
Proof with auto with real arith.
 intros.
 pose proof (atan2_spec (polar r s)).
 rewrite length_polar in H0...
 revert H0.
 generalize (atan2 (polar r s)).
 unfold polar, V2mult.
 simpl.
 intros.
 inversion H0.
 clear H0.
 rewrite Rabs_pos_eq in H2, H3...
 apply sameAngle_by_sincos; apply (Rmult_eq_reg_r s)...
Qed.

Lemma sameAngle_Aop x y:
  sameAngle x y -> sameAngle (Aop x) (Aop y).
Proof.
 intros [??].
 exists x0.
 subst.
 unfold Aop.
 ring.
Qed.

Lemma atan2_V2opp v:
  v <> V2zero ->
  sameAngle (atan2 (V2opp v)) (Aop (atan2 v)).
Proof with auto.
 intros.
 rewrite <- (atan2_spec v).
 assert (0 < length v)%R.
  apply positive_length...
 revert H0.
 generalize (atan2 v) (length v).
 intros.
 rewrite V2opp_polar.
 rewrite <- polar_Aop.
 apply sameAngle_trans with (Aop r).
  apply atan2_polar...
 apply sameAngle_sym, sameAngle_Aop, atan2_polar...
Qed.

Lemma V3plus_V3opp v:
  V3plus v (V3opp v) = v3 0 0 0.
Proof.
 apply V3e; repeat intro; simpl; ring.
Qed.


Lemma yrotate_mod (a b: R) (v: V3):
  sameAngle a b ->
  yrotate a v = yrotate b v.
Proof with auto.
 intros.
 apply V3e.
   apply Rminus_diag_uniq.
   simpl.
   rewrite (cos_sameangle_proper a b H).
   rewrite (sin_sameangle_proper a b H).
   ring.
  reflexivity.
 apply Rminus_diag_uniq.
 simpl.
 rewrite (cos_sameangle_proper a b H).
 rewrite (sin_sameangle_proper a b H).
 ring.
Qed.

Lemma rotate_polar a b l:
  rotate a (polar b l) = polar (a + b) l.
Proof with ring.
 unfold polar, rotate, V2mult.
 f_equal; simpl.
  rewrite sin_plus...
 rewrite cos_plus...
Qed.

Lemma length_rotate a v:
  length (rotate a v) = length v.
Proof with auto.
 rewrite <- (atan2_spec v).
 generalize (atan2 v).
 generalize (length v).
 intros.
 rewrite rotate_polar.
 repeat rewrite length_polar...
Qed.

Definition with_y (y: R) (v: V2): V3 := v3 (x2 v) y (y2 v).

Lemma atan2xz_spec v:
  v = with_y (y3 v) (polar (atan2 (xz v)) (length (xz v))).
Proof with auto.
 intros.
 unfold with_y.
 pose proof (atan2_spec (xz v)).
 revert H.
 generalize (atan2 (xz v)).
 intros.
 simpl.
 set (length (xz v)) in *.
 destruct v.
 inversion H...
Qed.

Lemma polar_eq a b n:
  (n <> 0)%R ->
  polar a n = polar b n ->
  sameAngle a b.
Proof with auto.
 intros.
 unfold polar in H0.
 inversion H0.
 clear H0.
 apply sameAngle_by_sincos.
  apply Rmult_eq_reg_r with n...
 apply Rmult_eq_reg_r with n...
Qed.

Lemma length_0 v:
  length v = 0%R -> v = V2zero.
Proof with auto with arith real.
 unfold length.
 destruct v.
 simpl.
 intros.
 assert (Rsqr x4 + Rsqr y4 = 0)%R.
  apply sqrt_eq_0...
  apply Rplus_le_le_0_compat...
 destruct (Rplus_sqr_eq_0 _ _ H0).
 subst...
Qed.

Lemma atan2_rotate v a:
  length v <> 0%R ->
  sameAngle (atan2 (rotate a v)) (atan2 v + a).
Proof with auto.
 intros.
 pose proof (atan2_spec v).
 set (atan2 v) in *. clearbody r.
 pose proof (atan2_spec (rotate a v)).
 set (atan2 (rotate a v)) in *. clearbody r0.
 rewrite length_rotate in H1.
 set (n := length v) in *.
 clearbody n.
 subst v.
 rewrite rotate_polar in H1.
 apply polar_eq with n...
 rewrite Rplus_comm...
Qed.

Lemma sameAngle_inv a b:
  sameAngle a b ->
  sameAngle (-a) (-b).
Proof with auto.
 unfold sameAngle.
 unfold Req_mod.
 intros [??].
 subst.
 exists (-x)%Z.
 ring_simplify.
 rewrite opp_IZR.
 ring.
Qed.

Lemma sameAngle_eq_reg_r a b c:
  sameAngle (a + c) (b + c) ->
  sameAngle a b.
Proof with auto.
 intros [??].
 exists x.
 apply Rplus_eq_reg_l with c.
 replace (c + b)%R with (b + c)%R by ring.
 rewrite <- H.
 ring.
Qed.

Instance yrotate_proper: Proper (sameAngle ==> eq) yrotate.
Proof.
 repeat intro.
 subst.
 apply ext_eq.
 intros.
 apply yrotate_mod.
 assumption.
Qed.

Instance yrotate_proper_ext: Proper (sameAngle ==> eq ==> eq) yrotate.
Proof.
 repeat intro.
 subst.
 apply yrotate_mod.
 assumption.
Qed.

Instance Ropp_proper: Proper (sameAngle ==> sameAngle) Ropp.
Proof.
 repeat intro.
 apply sameAngle_inv.
 assumption.
Qed.

Lemma V3mirror_yrotate: forall r p, V3mirror (yrotate r (V3mirror (yrotate r p))) = p.
Proof with try ring.
 intros.
 apply V3e; simpl; ring_simplify...
  apply eq_trans with (x3 p * (sin r ^ (2*1) + cos r ^ (2*1)))%R.
   simpl...
  do 2 rewrite pow_Rsqr.
  simpl.
  do 2 rewrite Rmult_1_r.
  rewrite sin2_cos2...
 apply eq_trans with (z3 p * (sin r ^ (2*1) + cos r ^ (2*1)))%R.
  simpl...
 do 2 rewrite pow_Rsqr.
 simpl.
 do 2 rewrite Rmult_1_r.
 rewrite sin2_cos2...
Qed.

Lemma V3mirror_V3plus v w: V3mirror (V3plus v w) = V3plus (V3mirror v) (V3mirror w).
Proof.
 apply V3e; simpl; ring.
Qed.

Lemma V3opp_V3mirror v: V3opp (V3mirror v) = V3mirror (V3opp v).
Proof.
 reflexivity.
Qed.


Lemma length_V2mirror v: length (V2mirror v) = length v.
Proof with auto.
 unfold length.
 unfold V2mirror.
 simpl.
 rewrite <- Rsqr_neg...
Qed.

Lemma V2mirror_polar a r: V2mirror (polar a r) = polar (- a) r.
Proof with auto.
 unfold polar.
 unfold V2mirror, V2mult.
 simpl.
 rewrite sin_neg, cos_neg.
 f_equal; ring.
Qed.

Lemma atan2_mirror v:
 v <> V2zero -> sameAngle (atan2 (V2mirror v)) (- atan2 v).
Proof with auto with real arith.
 intros.
 pose proof (atan2_spec v).
 set (atan2 v) in *. clearbody r.
 pose proof (atan2_spec (V2mirror v)).
 set (atan2 (V2mirror v)) in *.
 clearbody r0.
 rewrite length_V2mirror in H1.
 set (length v) in *.
 clearbody r1.
 subst v.
 rewrite V2mirror_polar in H1.
 apply polar_eq with r1...
 intro.
 apply H.
 subst.
 rewrite polar_0...
Qed.

Lemma yrotate_Ropp r v:
  yrotate (- r) v = V3mirror (yrotate r (V3mirror v)).
Proof.
 unfold yrotate, V3mirror, M3mult. simpl.
 f_equal; try rewrite cos_neg, sin_neg; ring.
Qed.

Lemma yrotate_V3mirror r v:
  yrotate r (V3mirror v) = V3mirror (yrotate (- r) v).
Proof.
 unfold yrotate, V3mirror, M3mult. simpl.
 f_equal; try rewrite cos_neg, sin_neg; ring.
Qed.

Lemma V2mult_V2mirror v s:
  V2mult (V2mirror v) s = V2mirror (V2mult v s).
Proof.
 unfold V2mult, V2mirror. simpl. f_equal; ring.
Qed.

Lemma V3mult_V3mirror v s:
  V3mult (V3mirror v) s = V3mirror (V3mult v s).
Proof.
 unfold V3mult, V3mirror. simpl. f_equal; ring.
Qed.

Lemma V3mirror_involutive v:
  V3mirror (V3mirror v) = v.
Proof.
 unfold V3mirror. simpl.
 rewrite Ropp_involutive.
 destruct v. reflexivity.
Qed.
