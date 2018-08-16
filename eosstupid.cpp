#include "./eosstupid.hpp"
#include <eosio.token/eosio.token.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/action.hpp>        // for SEND_INLINE_ACTION
#include <cmath> // for pow
// #include <boost/algorithm/string.hpp> // for split

#define CORE_SYMBOL S(4,EOS) // MainNet + JungleTestNet use EOS

using namespace eosio;
using namespace std;

inline uint64_t makeIndex(uint64_t stupidTurnOrder, uint8_t stupidOrder)
{
    return (stupidTurnOrder << 8) | stupidOrder;
}

inline uint64_t indexToStupidTurnOrder(uint64_t stupidOrder)
{
    return stupidOrder >> 8;
}

inline uint8_t indexToStupidOrder(uint64_t stupidOrder)
{
    return stupidOrder & 0xFF;
}

asset stupidofeos::contractbalance()
{
    return eosio::token(N(eosio.token)).get_balance(_self, eosio::symbol_type(CORE_SYMBOL).name());
    // return asset{0, CORE_SYMBOL};
}

inline void sendreward(account_name from,account_name to, asset reward){
    action transfers(
        permission_level{from, N(active)},
        N(eosio.token), N(transfer),
        std::make_tuple(from, to, reward, std::string("")));
    transfers.send();
}

void stupidofeos::sendstupidreward(asset reward,uint64_t count){
    if(count>0){
        auto stupid_itr = stupids.begin();
        eosio_assert(stupid_itr != stupids.end(),"data wrong!");
        for(; stupid_itr != stupids.end(); stupid_itr++){
            if(stupid_itr->name != _self){
            sendreward(_self,stupid_itr->name,asset{(int64_t)(reward.amount/count),CORE_SYMBOL});
            }
        }
    }
}

void stupidofeos::onTransfer(const currency::transfer &transfer)
{
    if (transfer.to != _self)
        return;

    eosio_assert(transfer.from != _self, "deployed contract may not take part in claiming the stupid");

    // print("Transfer memo: ", transfer.memo.c_str());
    eosio_assert(transfer.quantity.symbol == CORE_SYMBOL, "must pay with EOS token");
    auto itr = claims.end();
    --itr; // itr now points to last element
    eosio_assert(itr != claims.end(), "no previous claim exists");
    claim_record latestClaimRecord = *itr;

    uint64_t lastStupidTurnOrder = indexToStupidTurnOrder(latestClaimRecord.stupidIndex);
    uint8_t lastStupidOrder = indexToStupidOrder(latestClaimRecord.stupidIndex);

    eosio_assert(transfer.quantity.amount >= latestClaimRecord.price.amount * CLAIM_LEAST_MULtIPLIER, "claim price is too low");
    eosio_assert(transfer.quantity.amount <= latestClaimRecord.price.amount * CLAIM_MOST_MULtIPLIER, "claim price is too high");

    std::vector<std::string> results;

    claims.emplace(_self, [&](claim_record &claimRecord) {
        uint64_t stupidIndex = makeIndex(lastStupidTurnOrder, lastStupidOrder + 1);
        claimRecord.stupidIndex = stupidIndex;
        claimRecord.claimTime = now();
        claimRecord.name = transfer.from;
        claimRecord.memo = transfer.memo;
        claimRecord.price = transfer.quantity;
    });

    // first stupid is always deployed contract itself => cannot send transfer from itself to itself
    if (latestClaimRecord.name != _self)
    {
        asset amount = asset{(int64_t)(transfer.quantity.amount-(transfer.quantity.amount * COMMISSION_PERCENTAGE_POINTS)), CORE_SYMBOL};
        action{
            permission_level{_self, N(active)},
            N(eosio.token),
            N(transfer),
            currency::transfer{
                .from = _self, .to = latestClaimRecord.name, .quantity = amount, .memo = "You were fail to become the Stupid of EOS"}}
            .send();
        asset teamamount = asset{(int64_t)(transfer.quantity.amount * TEAM_PERCENTAGE_POINTS), CORE_SYMBOL};
        action{
            permission_level{_self, N(active)},
            N(eosio.token),
            N(transfer),
            currency::transfer{
                .from = _self, .to = TEAM_NAME, .quantity = teamamount, .memo = "TEAM LUCK"}}
            .send();
    }
}

void stupidofeos::end()
{
    // anyone can end the round, require no auth
    auto itr = claims.end();
    --itr; // itr now points to last element
    eosio_assert(itr != claims.end(), "no previous claim exists");

    time lastClaimTime = itr->claimTime;
    eosio_assert(now() > lastClaimTime + MAX_CORONATION_TIME, "max coronation time not reached yet");

    uint64_t lastStupidTurnOrder = indexToStupidTurnOrder(itr->stupidIndex);
    asset contractamount = contractbalance();

    sendstupidreward(asset{(int64_t)(contractamount.amount*STUPID_ROYALTY),CORE_SYMBOL}, lastStupidTurnOrder);
        
    stupids.emplace(_self, [&](stupid &stupid) {
        stupid.id = stupids.available_primary_key();
        stupid.name = itr->name;
    });

    claims.emplace(_self, [&](claim_record &claimRecord) {
        uint64_t stupidIndex = makeIndex(lastStupidTurnOrder + 1, 0);
        claimRecord.stupidIndex = stupidIndex;
        claimRecord.claimTime = now();
        claimRecord.name = _self;
        claimRecord.price = asset{10000,CORE_SYMBOL};
    });
}

void stupidofeos::init()
{
    require_auth(_self);
    // make sure table claims is empty
    eosio_assert(claims.begin() == claims.end(), "already initialized");
    claims.emplace(_self, [&](claim_record &claimRecord) {
        claimRecord.stupidIndex = makeIndex(0, 0);
        claimRecord.claimTime = now();
        claimRecord.name = _self;
        claimRecord.price = asset{10000, CORE_SYMBOL};
    });
}

void stupidofeos::apply(account_name contract, account_name act)
{
    if (contract == N(eosio.token) && act == N(transfer))
    {
        onTransfer(unpack_action_data<currency::transfer>());
        return;
    }

    if (contract != _self)
        return;

    // needed for EOSIO_API macro
    auto &thiscontract = *this;
    switch (act)
    {
        // first argument is name of CPP class, not contract
        EOSIO_API(stupidofeos, (init)(end))
    };
}

// EOSIO_ABI only works for contract == this conract
// EOSIO_ABI(stupidofeos, (transfer))

extern "C"
{
    [[noreturn]] void apply(uint64_t receiver, uint64_t code, uint64_t action) {
        stupidofeos stupid(receiver);
        stupid.apply(code, action);
        eosio_exit(0);
    }
}