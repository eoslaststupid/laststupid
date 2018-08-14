#include <string>

#include <eosiolib/eosio.hpp>
#include <eosiolib/currency.hpp>


// the time after which a new round begins when no
// new king was crowned
// 1 week
#define MAX_CORONATION_TIME (60 * 60 * 12)
#define CLAIM_LEAST_MULtIPLIER ((double)(1.1))
#define CLAIM_MOST_MULtIPLIER (3)
#define COMMISSION_PERCENTAGE_POINTS ((double)(0.05))
#define STUPID_PERCENTAGE_POINTS ((double)(0.03))
#define TEAM_PERCENTAGE_POINTS ((double)(0.02))
#define STUPID_ROYALTY ((double)(0.7))
#define TEAM_NAME N(eosiostupid)

using eosio::asset;

class stupidofeos : public eosio::contract
{
  public:
    stupidofeos(account_name self)
        : contract(self),
          claims(self, self),
          stupids(self, self)
    {
    }

    //@abi table claims i64
    struct claim_record
    {
        claim_record(){};
        claim_record(uint64_t stupidIndex, time claimTime, account_name name,std::string memo, asset price)
            : stupidIndex(stupidIndex), claimTime(claimTime), name(name), memo(memo), price(price){};
        // upper 56 bits contain kingdom order, lower 8 bits contain kingOrder
        uint64_t stupidIndex; // this also acts as key of the table
        time claimTime;
        account_name name;
        std::string memo;
        asset price;

        uint64_t primary_key() const { return stupidIndex; }
        // need to serialize this, otherwise saving it in the data base does not work
        // Runtime Error Processing WASM
        EOSLIB_SERIALIZE(claim_record, (stupidIndex)(claimTime)(name)(memo)(price))
    };

    //@abi table stupids i64
    struct stupid
    {
        
        uint64_t id; 
        account_name name;

        uint64_t primary_key() const { return id; }
        // need to serialize this, otherwise saving it in the data base does not work
        // Runtime Error Processing WASM
        EOSLIB_SERIALIZE(stupid, (id)(name))
    };

    //@abi action init
    struct init
    {
        init(){};
        // action must have a field as of now
        account_name name;
        EOSLIB_SERIALIZE(init, (name))
    };

    //@abi action end
    struct end
    {
        end(){};
        // action must have a field as of now
        account_name name;
        EOSLIB_SERIALIZE(end, (name))
    };

    // the first argument of multi_index must be the name of the table
    // in the ABI!
    typedef eosio::multi_index<N(claims), claim_record> claims_db;
    typedef eosio::multi_index<N(stupids), stupid> supid_db;

    void onTransfer(const eosio::currency::transfer& transfer);
    void end ();
    void init();
    void apply( account_name contract, account_name act );

  private:
    claims_db claims;
    supid_db stupids;
    void sendstupidreward(asset reward,uint64_t count);
    asset contractbalance();
};