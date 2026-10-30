#pragma once
class Character;
// Visual confirmation of already-completed payment. Never mutates money/items.
void DfShowTradeReceipt(Character* actor,int paid);
