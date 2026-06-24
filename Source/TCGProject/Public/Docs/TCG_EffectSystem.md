# TCG Effect System

This file expands the effect rules from `TCG_GameRules.md` into implementation-facing notes.

## Current Goal

The effect system should no longer be only `Trigger + EffectId`.

A card effect is built from reusable pieces:

* trigger piece
* condition / validity piece
* target piece
* action piece
* value piece
* selection piece
* flow piece

Example card text:

> When played, draw 2 cards, then discard 1 and increase this card's Attack by 2. If this card is destroyed, increase the Attack of one of your Units on board by 2.

This should be represented as separate effect entries:

```text
Effect A
Trigger: OnPlay
Steps:
  1. DrawCards, Controller, Value 2
  2. Then
  3. DiscardCards, Controller, Value 1, SelectionMode PlayerChoice, RequiresPreviousStepSuccess
  4. Then
  5. ModifyAttack, SourceCard, ValueMode Fixed, Value 2, RequiresPreviousStepSuccess

Effect B
Trigger: OnDestroyed
Steps:
  1. SelectTarget, Controller board Unit
  2. ModifyAttack, SelectedTarget, ValueMode Fixed, Value 2, RequiresPreviousStepSuccess
```

## Data Shape

`FTCGCardEffectRef` has:

* `Trigger`
* `EffectId`
* `Steps`
* `bOptional`

`EffectId` is now only a temporary migration/debug field. Existing old debug ids are converted into modular steps before they enter the chain.

For real card effects going forward, the preferred path is modular `Steps`.

Effects with no modular steps should not resolve.

## Step Types

Initial reusable step types:

* `Then`
* `DrawCards`
* `DiscardCards`
* `ModifyAttack`
* `SelectTarget`
* `MoveBottomOverlayToGraveyard`

`Then` is a flow step. It does not perform a gameplay action by itself.

A gameplay step can use `bRequiresPreviousStepSuccess` to mean:

> only resolve this step if the previous meaningful step succeeded.

This allows effects such as:

```text
Draw 2 cards, then discard 1.
```

If the draw step fails, the discard step can be skipped.

## Selection Modes

Some steps can either happen automatically or ask the player.

Implemented selection modes:

* `Automatic`
* `PlayerChoice`

For `DiscardCards`:

```text
SelectionMode Automatic
```

means the system discards cards from hand using the current automatic order.

```text
SelectionMode PlayerChoice
```

means the resolver creates `PendingDiscardChoice` on the GameState. UI/Blueprint can read the available card instance ids through `GetPendingDiscardChoiceOptions` and complete the choice with `SubmitPendingDiscardChoice`.

Current limitation:

A pending discard choice does not yet resume later chain steps after the choice is submitted. For effects ending at discard, such as `Draw 2, then choose and discard 1`, this is enough to test the player-choice path. Later chain-resume support should store the paused chain entry and next step index.

## Value Modes

Some steps need a number.

`FTCGEffectStep::ValueMode` controls how that number is produced.

Implemented value modes:

* `Fixed`
* `CardsUnderneathSource`
* `CardsUnderneathTarget`

Examples:

```text
ModifyAttack, SourceCard, ValueMode Fixed, Value 2
```

means:

```text
This card gains +2 Attack.
```

```text
ModifyAttack, TriggerTarget, ValueMode CardsUnderneathTarget
```

means:

```text
The played/top card gains Attack equal to the number of cards underneath it.
```

## Chain Build Order

The rules from `TCG_GameRules.md` remain unchanged.

When a gameplay event happens, valid effects are collected and added to an effect chain.

The chain is built in trigger order.

For inherited stack effects, the default order is bottom card to top card.

Example stack:

* bottom card: On Play Draw 1
* middle card: On Play Add 1 card from Graveyard to hand
* top card: On Play Gain Attack

Chain build order:

* Chain 1: bottom card effect
* Chain 2: middle card effect
* Chain 3: top card effect

## Chain Resolution Order

The chain resolves backward.

Example:

* Chain 3 resolves first
* Chain 2 resolves second
* Chain 1 resolves last

This is important because response effects added later should resolve before earlier effects.

## Resolution Validity / Fizzle

Adding an effect to the chain does not guarantee it resolves.

When a chain entry resolves, the game must re-check validity:

* source card still exists
* source is still in the required location/state
* target still exists
* target is still legal
* inherited overlay source is still under the target if required

If validity fails, the chain entry fizzles.

A fizzled effect applies no result.

## Logging Rules

Effect logs should be useful but not spammy.

Log these moments:

* effect added to chain
* chain entry starts resolving
* chain entry fizzles
* modular effect starts resolving
* each used step succeeds/fails
* player choice is requested
* player choice is submitted
* step skipped because a previous required step failed

Do not log every passive scan, every card in every zone, or every tick-like check.

The log should read like a replay skeleton, not a haystack.

## Current Implementation Status

Implemented data:

* `ETCGEffectStepType`
* `ETCGEffectTargetMode`
* `ETCGEffectValueMode`
* `ETCGEffectSelectionMode`
* `FTCGEffectTargetFilter`
* `FTCGEffectStep`
* `FTCGCardEffectRef::Steps`
* `FTCGCardEffectRef::bOptional`
* `FTCGEffectChainEntry::EffectRef`
* `FTCGPendingDiscardChoice`

Implemented resolver helpers:

* `AddCardEffectRefToChain`
* `ResolveEffectChainEntry`
* `ResolveModularEffectChainEntry`
* `ResolveEffectStep`
* `DiscardCardsFromHand`
* `BeginPendingDiscardChoice`
* `SubmitPendingDiscardChoice`
* `MoveBottomOverlayToGraveyard`

Implemented first modular steps:

* DrawCards
* DiscardCards
* ModifyAttack
* SelectTarget scaffold
* MoveBottomOverlayToGraveyard

Implemented chain hook:

* `BuildStackOnPlayEffectChain` uses `GetPrintedEffectRefsForCard` and `AddCardEffectRefToChain`.
* `ResolveEffectChain` calls `ResolveEffectChainEntry`, which now expects modular steps.

Legacy cleanup status:

* `Debug_Draw1` is auto-converted to modular `DrawCards 1` when added to the chain.
* `Debug_GainAttackForCardsUnderneath` is auto-converted to modular `ModifyAttack` with `CardsUnderneathTarget` when added to the chain.
* `Debug_RemoveBottomOverlay` is auto-converted to modular `MoveBottomOverlayToGraveyard` when added to the chain.
* The old resolver path is no longer used by active chain resolution.
* Some deprecated compatibility declarations remain because old definitions still exist in `TCG_GameState.cpp`; they should be removed in a future file cleanup pass.
