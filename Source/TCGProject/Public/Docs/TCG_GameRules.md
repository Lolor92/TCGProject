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

The match does not end because a player has no cards in hand or deck.

If a player has no card to place during placement, that placement step is skipped for now.

## Round Limit Win Condition

A match has a maximum round number.

By default, the maximum round number is 10.

The maximum round number is a game rule setting and can be changed in Blueprint through the GameState defaults.

This allows different game modes to use different round limits.

After the battle phase of the maximum round, if both players still have Units on the board, the winner is decided by board Unit count.

A board Unit means one stack on the board. A stacked Unit still counts as one Unit, even if it contains multiple cards.

Round-limit result:

* If Player 0 has more board Units, Player 0 wins.
* If Player 1 has more board Units, Player 1 wins.
* If both players have the same number of board Units, the match is a draw.

The normal no-Units lose condition is still checked first after battle.

The round-limit Unit-count rule only applies if both players still have at least one Unit after the battle phase.

## Round Structure

The match is played in rounds.

A round is one full cycle where both players place cards onto the board through alternating placement steps, then battle resolves.

A round contains:

* Round Start
* Placement Phase
* Battle Phase
* Round End

During the Placement Phase, players do not each take a full traditional turn. Instead, they alternate placement steps.

If no player loses after the Battle Phase and the maximum round has not been reached, the match proceeds to the next round.

## Placement Phase

During the Placement Phase, players take turns placing cards onto their board.

A placement step is one player's chance to place one card.

By default, Player 0 takes the first placement step of the round.

The placement order is:

* Placement Step 1: Player 0 places 1 card.
* Placement Step 2: Player 1 draws 1 card, then places 1 card.
* Placement Step 3: Player 0 draws 1 card, then places 1 card.
* Placement Step 4: Player 1 draws 1 card, then places 1 card.
* This continues until both players have had 4 placement steps.

After Player 1 completes the 8th total placement step of the round, the Placement Phase ends and the Battle Phase begins.

The first player does not draw at the start of the first placement step of the round unless a card effect or future rule says otherwise.

If a player cannot draw because their deck is empty, the placement step still continues.

If a player cannot place because they have no legal card or no legal empty field zone, that placement step is skipped for now.

The match still only checks win/loss conditions after the Battle Phase.

## Field Zones and Placement Order

Each player has 4 field zones.

By default, cards must be placed into field zones in order.

The normal placement order is:

* Field Zone 1
* Field Zone 2
* Field Zone 3
* Field Zone 4

A player cannot normally place a card into Field Zone 2 while Field Zone 1 is empty.

A player cannot normally place a card into Field Zone 3 while Field Zone 1 or Field Zone 2 is empty.

A player cannot normally place a card into Field Zone 4 while Field Zone 1, Field Zone 2, or Field Zone 3 is empty.

Default normal placement should be into the next empty field zone in order.

Stacking onto an already-filled earlier zone should not be allowed by normal placement once a later empty zone is next, unless a card effect or rule modifier allows it.

Some card effects may allow exceptions to the normal placement order.

Examples of possible future exceptions:

* Place a card into any empty field zone.
* Skip the next required field zone.
* Place a card directly on top of a specific Unit.
* Move a Unit to another field zone.
* Open an extra placement zone for this round.

These exceptions should be handled through card effects or explicit rule modifiers, not by changing the default placement rule.

## Placement Step Completion

A placement step normally ends after the active player places one card.

If the active player cannot place a card, the placement step is skipped for now.

When both players have completed or skipped 4 placement steps each, the round proceeds to the Battle Phase.

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

Cards underneath the top card are overlay cards/materials.

They contribute to the top card, but they do not count as separate Units on the board.

For win/loss checks and round-limit Unit counting, one stack on the board counts as one Unit.

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

During battle, Units attack in field-zone order.

The top card of each stack is used as the battling Unit.

Each battling Unit uses its final Attack value.

Final Attack includes the stack attack bonus from cards underneath it.

The attacking player alternates by zone:

* Field Zone 1: Player 0's Unit declares the attack.
* Field Zone 2: Player 1's Unit declares the attack.
* Field Zone 3: Player 0's Unit declares the attack.
* Field Zone 4: Player 1's Unit declares the attack.

If the expected attacking Unit is missing from that field zone, that attack declaration is skipped.

When an attack is declared, the defending target is chosen from the opponent's board.

The attack first checks the opposing field zone directly across from the attacking Unit.

If there is no Unit directly across, the attack checks the next opponent field zone in order.

If the search reaches the end of the opponent's field zones, it wraps back to Field Zone 1.

Examples:

* If Player 0's Field Zone 1 Unit attacks and Player 1 has no Unit in Field Zone 1, the attack checks Player 1 Field Zone 2, then Field Zone 3, then Field Zone 4.
* If Player 0's Field Zone 3 Unit attacks and Player 1 has no Unit in Field Zone 3 or Field Zone 4, the attack checks Player 1 Field Zone 1, then Field Zone 2.

Battle comparison:

* Higher final Attack wins.
* Lower final Attack loses and its whole stack is sent to the Graveyard.
* If both final Attack values are equal, both stacks are sent to the Graveyard.

After all battle resolution is finished, win/loss conditions are checked.

## Battle Effect Timing

When a Unit declares an attack, effects that trigger on attacking can be added to the chain.

If the attacked Unit has effects that trigger when it is attacked, those effects can be added after the attacker's effects.

Example:

* Player 0 Field Zone 1 Unit declares an attack.
* That Unit has an On Attack effect, so it can be added as Chain 1.
* The attacked Unit has an On Attacked effect, so it can be added as Chain 2.
* The chain resolves backward before the final battle comparison if the effect timing says it should resolve before damage/battle comparison.

The exact attack-trigger effect system will be implemented later.

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
* On Attack
* On Attacked

These are design notes for now.

Full effect logic will be implemented later.

## Effect Chains

Effects do not always resolve immediately.

When a gameplay event happens, such as playing a Unit or declaring an attack, the game collects valid effects that trigger from that event and builds an effect chain.

The chain is built in trigger order, then resolves backward from the newest chain entry to the oldest chain entry.

Example:

* Chain 1 is added first.
* Chain 2 is added second.
* Chain 3 is added third.

The chain resolves backward:

* Chain 3 resolves first.
* Chain 2 resolves second.
* Chain 1 resolves last.

## Mandatory and Optional Effects

Some effects are mandatory.

Some effects are optional.

If a mandatory effect meets its trigger and condition requirements, it must be added to the chain.

If an optional effect meets its trigger and condition requirements, the controller of that effect chooses whether to add it to the chain.

Optional effects are usually written with language such as:

* You can...
* You may...
* You can choose to...

Example:

* A player controls 2 Units.
* Both Units have effects that say: When your opponent plays a Unit, you can activate this effect.
* The opponent plays a Unit.
* Both effects meet their trigger requirements.
* The player can choose to activate neither effect, only one effect, or both effects.

If the player chooses to activate both effects, that player chooses the order those effects are added to the chain.

## Player Choice for Chain Order

When a player has multiple valid effects that can be added to the chain at the same timing, that player chooses the order they are added.

Example:

* Player controls Dark Monster 1.
* Player controls Fire Monster 1.
* Both have effects that can trigger when the opponent plays a Unit.

The player may choose:

* Chain 1: Dark Monster 1.
* Chain 2: Fire Monster 1.

If no more effects are added, the chain resolves backward:

* Chain 2 resolves first.
* Chain 1 resolves second.

The player could also choose the opposite order:

* Chain 1: Fire Monster 1.
* Chain 2: Dark Monster 1.

Then the chain would resolve backward:

* Chain 2 resolves first.
* Chain 1 resolves second.

This means the order chosen during chain building affects the order effects resolve.

## Stack Chain Order

When a Unit is played on top of an existing stack, valid On Play effects from the stack are added to the chain from bottom card to top card by default.

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

This bottom-to-top order is the default stack order for inherited stack effects.

If optional effects are involved, the controller still chooses whether to add them when allowed by the rules.

## Opponent Responses

Opponent effects can also be added to the chain if their trigger and condition requirements are met.

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

## Priority

The turn player has priority when adding effects to the chain.

When a gameplay event happens, the turn player gets the first chance to add valid mandatory and optional effects to the chain.

After the turn player adds their effects, the opponent gets a chance to add valid mandatory and optional effects to the chain.

This can continue as long as players have valid effects to add.

When a player has multiple optional effects that meet their trigger and condition requirements, that player chooses:

* whether to activate each optional effect
* which optional effects to activate
* the order those effects are added to the chain

## Priority Example

Player 0 is the turn player.

Player 0 plays a Unit with this effect:

* On Play: Draw 1 card.

Player 1 controls 2 Units:

* Dark Monster 1: When your opponent plays a Unit, you can reduce that Unit's Attack by 1.
* Fire Monster 1: When your opponent plays a Unit, you can reduce that Unit's Attack by 2.

Because Player 0 is the turn player, Player 0 has priority.

The chain starts:

* Chain 1: Player 0's played Unit draws 1 card.

Player 1 then chooses whether to add their optional response effects.

Player 1 can choose to add both effects and choose their order:

* Chain 2: Dark Monster 1 reduces Attack by 1.
* Chain 3: Fire Monster 1 reduces Attack by 2.

If no more effects are added, the chain resolves backward:

* Chain 3 resolves first.
* Chain 2 resolves second.
* Chain 1 resolves last.

Player 1 could also choose the opposite order:

* Chain 2: Fire Monster 1 reduces Attack by 2.
* Chain 3: Dark Monster 1 reduces Attack by 1.

Then Dark Monster 1 resolves first because it was added last.

## Chain Resolution Validity

A chain entry must still be valid when it resolves.

Adding an effect to the chain does not guarantee that the effect will resolve successfully.

When a chain entry tries to resolve, the game should re-check important requirements such as:

* the source card still exists
* the source is still in the required place or state
* the target still exists
* the target is still a legal target
* any other required condition is still true

If the chain entry no longer meets its resolution requirements, the effect fizzles.

A fizzled effect does not apply its result.

## Fizzle Example: Removing an Overlay Before Its Effect Resolves

Player 0 plays a Unit on top of another Unit.

The bottom Unit has this inherited effect:

* On Play: Draw 1 card.

That effect is added to the chain:

* Chain 1: Bottom overlay card Draw 1.

Player 1 controls a Unit with this response effect:

* When your opponent plays a Unit, you can remove 1 card underneath that Unit.

Player 1 adds the response:

* Chain 2: Remove 1 overlay card from underneath the played Unit.

The chain resolves backward.

Chain 2 resolves first:

* Player 1 removes the bottom overlay card from underneath Player 0's Unit.
* That removed card was the source of Chain 1's Draw 1 effect.

Then Chain 1 tries to resolve:

* The source card of Chain 1 is no longer underneath the Unit.
* The source card no longer meets the required state for the inherited effect.
* Chain 1 fizzles.
* Player 0 does not draw 1 card.

This means inherited overlay effects can be interrupted before they resolve.

## Chain Limit

There is no fixed chain limit.

The chain can keep growing as long as valid effects meet their trigger and condition requirements.

A chain only starts resolving after both players have finished adding effects.

## Replay Note

Effect chains should be represented as recorded match actions later.

A replay system must be able to recreate:

* which effects were eligible
* which optional effects were chosen
* which optional effects were skipped
* which player made each choice
* which effects were added to the chain
* the order effects were added
* the order effects resolved
* which effects fizzled
* why each effect fizzled
* the result of each resolved effect

For this reason, effect collection, chain building, player choices, chain resolution, and resolution validity checks should stay centralized instead of being hidden inside unrelated helper functions.

## Implementation Notes

The current implementation is still a debug skeleton.

The current goal is to prove:

* round flow
* ordered placement
* battle declaration order
* fallback battle target selection
* round-limit Unit-count result
* trigger timing
* stack chain build order
* reverse chain resolution order
* replay-friendly action chokepoints

The full effect engine will be implemented later.

Printed/static effect data should eventually come from `UTCG_CardDefinition::Effects`.

Runtime chain entries should eventually represent selected effects from that static data.

Effect execution should eventually be separated into these steps:

* detect gameplay event
* collect valid effects
* ask/resolve player choices for optional effects
* add chosen effects to the chain
* allow response effects
* resolve the chain backward
* re-check resolution validity for each chain entry
* fizzle invalid chain entries
* record all meaningful actions for replay
