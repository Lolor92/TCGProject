# TCG Project - Core Game Rules

## Game Identity

This is an original unit-only TCG.

There are no life points.
There are no spell cards.
There are no artifact cards.
All playable cards are Unit cards.

## Lose Condition

A player loses if they have no cards on their board after the battle phase.

The lose condition should be checked after battle resolution, not immediately when the board becomes empty.

## Card Elements

Each card has one element:

- Fire
- Wind
- Earth
- Water
- Light
- Dark

## Card Stats

Cards have Attack.

Cards do not have Defense.

## Card Stacking

Cards can be placed on top of other cards.

By default, a card can only be played on top of another card with the same element.

Example:

- Fire can be played on Fire.
- Water can be played on Water.
- Fire cannot normally be played on Water.

Some cards may have effects that create exceptions to this rule.

## Stack Attack Bonus

A card gains +1 Attack for each card underneath it in the same stack.

Example:

- Top card has 3 base Attack.
- It has 2 cards underneath it.
- Final Attack is 5.

## Stack Visual Rule

When a card is played on top of another card, it covers about 75% of the card underneath.

The bottom part of the lower card should remain visible because that area shows the lower card's effect text.

## Inherited Effects

The top card can gain/use effects from cards underneath it.

Example:

- A Water card has an On Play effect: Draw 1 card.
- Another Water card is played on top of it.
- The new top card can use/trigger the effect from the card underneath.
- This means the draw effect can happen again.

## Effect Timing Ideas

Cards may have effects triggered by events such as:

- On Play
- On Destroyed
- On Sent To Graveyard
- On Banished
- On Battle Start
- On Battle End
- On Stack Added
- On Becoming Top Card

These are design notes for now. Full effect logic will be implemented later.