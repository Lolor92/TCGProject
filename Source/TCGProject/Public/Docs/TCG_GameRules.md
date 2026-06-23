# TCG Project - Core Game Rules

## Game Identity

This is an original unit-only TCG.

There are no life points.

There are no spell cards.

There are no artifact cards.

All playable cards are Unit cards.

## Lose Condition

A player loses if they have no Units on their board after the battle phase.

The lose condition is checked after battle resolution, not immediately when the board becomes empty during battle resolution.

If both players have no Units on their board after battle resolution, the match result is a draw.

## Card Elements

Each card has one element:

* Fire
* Wind
* Earth
* Water
* Light
* Dark

## Card Stats

Cards have Attack.

Cards do not have Defense.

## Board Units

A single unstacked card on the board counts as one Unit.

A stack of cards on the board also counts as one Unit.

Only the top card of a stack is treated as the active Unit on the board.

Cards underneath the top card are overlay cards/materials. They contribute to the top card, but they do not count as separate Units on the board.

## Card Stacking / Overlay Rules

Cards can be placed on top of other cards to form a stack.

By default, a card can only be played on top of another card with the same element.

Examples:

* Fire can be played on Fire.
* Water can be played on Water.
* Fire cannot normally be played on Water.
* Water cannot normally be played on Fire.

Some cards may have effects that create exceptions to this rule later.

## Stack Attack Bonus

The top card gains +1 Attack for each card underneath it in the same stack.

Example:

* Top card has 3 base Attack.
* It has 2 cards underneath it.
* Final Attack is 5.

The cards underneath are not separate Units, but they still count as cards underneath the top card for stack bonuses.

## Stack Death / Leaving Board Rule

A stack is treated as one Unit.

If the top card of a stack leaves the board, the entire stack leaves the board with it.

Examples:

* If the top card is sent to the Graveyard, all cards underneath it are also sent to the Graveyard.
* If the top card is Banished, all cards underneath it are also Banished.
* If the top card is destroyed in battle, the whole stack is removed from the board.
* If a stacked Unit loses battle, the whole stack is sent to the Graveyard.

Cards underneath do not remain on the board after the top card leaves.

## Battle Rules

During battle, opposing board zones can resolve against each other.

The top card of each stack is used as the battling Unit.

Each battling Unit uses its final Attack value.

Final Attack includes the stack attack bonus from cards underneath it.

Battle comparison:

* Higher final Attack wins.
* Lower final Attack loses and its whole stack is sent to the Graveyard.
* If both final Attack values are equal, both stacks are sent to the Graveyard.

After all battle resolution is finished, the lose condition is checked.

## Stack Visual Rule

When a card is played on top of another card, it covers about 75% of the card underneath.

The bottom part of the lower card should remain visible because that area shows the lower card's effect text.

## Inherited Effects

The top card can gain or use effects from cards underneath it.

Cards underneath contribute their effects to the top card, but they do not count as separate Units on the board.

Example:

* A Water card has an On Play effect: Draw 1 card.
* Another Water card is played on top of it.
* The new top card can use or trigger the effect from the card underneath.
* This means the draw effect can happen again.

The exact execution rules for inherited effects will be implemented later.

## Effect Timing Ideas

Cards may have effects triggered by events such as:

* On Play
* On Destroyed
* On Sent To Graveyard
* On Banished
* On Battle Start
* On Battle End
* On Stack Added
* On Becoming Top Card

These are design notes for now.

Full effect logic will be implemented later.

## Effect Chains

Effects do not always resolve immediately.

When a gameplay event happens, such as playing a Unit, the game collects all valid effects that trigger from that event and builds an effect chain.

The chain is built in trigger order, then resolves backward from the newest chain entry to the oldest chain entry.

### Stack Chain Order

When a Unit is played on top of an existing stack, valid On Play effects from the stack are added to the chain from bottom card to top card.

Example:

* Bottom card has On Play: Draw 1 card.
* Middle card has On Play: Add 1 card from Graveyard to hand.
* Top card has On Play: Gain Attack equal to the number of cards underneath it.

When the top card is played, the chain is built like this:

* Chain 1: Bottom card Draw 1.
* Chain 2: Middle card add 1 card from Graveyard to hand.
* Chain 3: Top card gain Attack.

The chain then resolves backward:

* Chain 3 resolves first.
* Chain 2 resolves second.
* Chain 1 resolves last.

### Opponent Responses

Opponent effects can also be added to the chain if their conditions are met.

Example:

* Turn player plays a Unit onto a stack.
* The stack creates Chain 1, Chain 2, and Chain 3.
* Opponent has an effect: When your opponent plays a Unit, reduce that Unit's Attack by 2.
* Opponent adds that effect as Chain 4.

If no more effects are added, the chain resolves backward:

* Chain 4 resolves first.
* Chain 3 resolves second.
* Chain 2 resolves third.
* Chain 1 resolves last.

### Priority

The turn player has priority when adding effects to the chain.

There is no fixed chain limit. The chain can keep growing as long as valid effects meet their trigger and condition requirements.

### Replay Note

Effect chains should be represented as recorded match actions later.

A replay system must be able to recreate:

* which effects were added to the chain
* the order they were added
* the order they resolved
* the result of each resolved effect

For this reason, effect collection, chain building, and chain resolution should stay centralized instead of being hidden inside unrelated helper functions.