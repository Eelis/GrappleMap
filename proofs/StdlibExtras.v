Require Import Reals Rtrigo1 Program.Basics Rsqrt_def Classes.Morphisms.

Axiom ext_eq: forall A B (f g: A -> B), (forall x, f x = g x) -> f = g.

Definition idd {T: Type} (x: T): T := x.

Lemma compose_idd_r A B (f: A -> B): compose f idd = f.
Proof.
 apply ext_eq. intro. reflexivity.
Qed.

Lemma sin_period_Z: forall x k, sin (x + 2 * IZR k * PI) = sin x.
Proof with ring.
 intros.
 destruct k; simpl.
   f_equal...
  apply sin_period.
 replace (x + 2 * - INR (Pos.to_nat p) * PI)%R
    with (-(-x + 2 * INR (Pos.to_nat p) * PI))%R by ring.
 rewrite sin_neg, sin_period, sin_neg...
Qed.

Lemma cos_period_Z: forall x k, cos (x + 2 * IZR k * PI) = cos x.
Proof with ring.
 intros.
 destruct k; simpl.
   f_equal...
  apply cos_period.
 replace (x + 2 * - INR (Pos.to_nat p) * PI)%R
    with (-(-x + 2 * INR (Pos.to_nat p) * PI))%R by ring.
 rewrite cos_neg, cos_period, cos_neg...
Qed.

Lemma pi_pos: (0 < PI)%R.
Proof.
 apply Rgt_lt, PI_RGT_0.
Qed.

Lemma pi_nonneg: (0 <= PI)%R.
Proof.
 apply Rlt_le, pi_pos.
Qed.

Lemma sameAngle_low_high: forall a b,
 (0 <= a -> a < PI ->
  PI <= b -> b < 2 * PI ->
  sin a = sin b ->
  a = 0 /\ b = PI)%R.
Proof with auto with real.
 intros.
 assert (sin b = 0)%R.
  apply Rle_antisym.
   apply sin_le_0...
  rewrite <- H3.
  apply sin_ge_0...
 assert (sin a = 0)%R.
  rewrite H3...
  clear H3.
 split.
  destruct (sin_eq_O_2PI_0 a H)...
   apply Rle_trans with (1 * PI)%R...
    ring_simplify...
   apply Rmult_le_compat...
   apply pi_nonneg.
  destruct H3.
   subst.
   exfalso.
   apply Rlt_irrefl with PI...
  subst.
  exfalso.
  apply (Rlt_asym (2 * PI) PI)...
  replace PI with (1 * PI)%R at 1 by ring.
  apply Rmult_lt_compat_r...
  apply pi_pos.
 destruct (sin_eq_O_2PI_0 b)...
   apply Rle_trans with PI...
   apply pi_nonneg.
  subst.
  exfalso.
  apply (Rle_not_lt _ _ H1).
  apply pi_pos.
 destruct H3...
 subst.
 exfalso.
 apply Rlt_irrefl with (2 * PI)%R...
Qed.


Lemma twopi_ne_0: (2 * PI <> 0)%R.
Proof.
 intro H.
 apply Rlt_irrefl with (2 * PI)%R.
 rewrite H at 1.
 apply Rgt_2PI_0.
Qed.

Lemma twopi_nonneg: (0 <= 2 * PI)%R.
Proof.
 apply Rmult_le_pos.
  apply (pos_INR 2).
 apply pi_nonneg.
Qed.

Lemma Rne_lt (x: R): x <> 0%R -> ({0 < x} + {x < 0})%R.
Proof with auto.
 intros.
 destruct (Rlt_le_dec x 0); [right | left]...
 destruct (Rle_lt_or_eq_dec _ _ r)...
 subst.
 exfalso...
Qed.

Lemma Ropp_ne x: x <> 0%R -> (-x <> x)%R.
Proof with auto.
 intros.
 intro.
 destruct (Rne_lt x H).
  apply (Rlt_asym x 0)...
  rewrite <- H0.
  apply Ropp_lt_gt_0_contravar...
 apply (Rlt_asym x 0)...
 rewrite <- H0.
 apply Ropp_0_gt_lt_contravar...
Qed.

Lemma compose_idd_l {A B} (f: A -> B): compose idd f = f.
Proof with auto.
 apply ext_eq.
 reflexivity.
Qed.

Lemma Rsqrt_0 x p: (Rsqrt (mknonnegreal x p) = 0 -> x = 0)%R.
Proof with auto.
 unfold Rsqrt.
 simpl.
 destruct Rsqrt_exists.
 destruct a.
 subst.
 intros.
 subst.
 compute.
 ring.
Qed.


