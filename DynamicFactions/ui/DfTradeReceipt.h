#pragma once
class Character;
// Visual confirmation of already-completed payment. Never mutates money/items.
// Positive = earned, negative = spent.
void DfShowTradeReceipt(Character* actor,int paid);
