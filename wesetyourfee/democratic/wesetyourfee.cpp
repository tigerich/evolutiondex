#include "wesetyourfee.hpp"

// using namespace setfee;

int wesetyourfee::median(symbol sym){
    feetables tables( get_self(), get_self().value );
    auto table = tables.find( sym.code().raw());
    check( table != tables.end(), "fee table nonexistent, run openfeetable" );
    auto votes = table->votes;
    int64_t sum = 0, partial_sum = 0;
    int index = 0;
    for (int i = 0; i < 9; i++) {
        sum += votes.at(i);
        print("votes at ", fee_vector.at(i), " es: ", votes.at(i), "\n");
    }
    print("sum es: ", sum," ");
    for (int i = 0; 2 * partial_sum < sum; i++) {
        partial_sum += votes.at(i);
        index = i;
    }
    return fee_vector.at(index);
}

void wesetyourfee::changefee(symbol sym, int newfee_index) {
    check( newfee_index == median(sym), "fee value not allowed" );
    require_recipient( "evolutiondex"_n );
}

void wesetyourfee::addliquidity(name user, asset to_buy, extended_asset max_ext_asset1, extended_asset max_ext_asset2){
    add_balance(user, to_buy);
}

void wesetyourfee::remliquidity(name user, asset to_sell, extended_asset min_ext_asset1, extended_asset min_ext_asset2){
    add_balance(user, -to_sell);
}

void wesetyourfee::transfer(const name& from, const name& to, const asset& quantity, const string&  memo ){
    add_balance(from, -quantity);
    add_balance(to, quantity);
};

asset wesetyourfee::bring_balance(name user, symbol sym) {
    accounts table( "evolutiondex"_n, user.value );
    const auto& user_balance = table.find( sym.code().raw() );
    check ( user_balance != table.end(), "token does not exist" );
    return user_balance->balance;
}

void wesetyourfee::votefee(name user, symbol sym, int fee_index_voted){
    require_auth(user);
    check( (0 <= fee_index_voted) && (fee_index_voted < 9), "index voted must be between 0 and 8");
    feeaccounts acnts( get_self(), user.value );
    auto acnt = acnts.find( sym.code().raw());
    auto balance = bring_balance(user, sym);
    print("balance es: ", balance, "\n");
    if ( acnt == acnts.end() ) {
        acnts.emplace( user, [&]( auto& a ){
          a.sym = sym;
          a.fee_index_voted = fee_index_voted;
        });
    } else {
        addvote(sym, acnt->fee_index_voted, -balance.amount );
        acnts.modify( acnt, user, [&]( auto& a ){
          a.fee_index_voted = fee_index_voted;
        });
    }
    addvote(sym, fee_index_voted, balance.amount);
}

void wesetyourfee::closevote(name user, symbol sym) {
    require_auth(user);
    feeaccounts acnts( get_self(), user.value );
    auto acnt = acnts.find( sym.code().raw());
    check( acnt != acnts.end(), "user is not voting" );
    auto balance = bring_balance(user, sym);    
    addvote( sym, acnt->fee_index_voted, -balance.amount );
    acnts.erase(acnt);  // tal vez tira assert si no existe, puedo ahorrar líneas
}

void wesetyourfee::openfeetable(name user, symbol sym) {
    feetables tables( get_self(), get_self().value );
    auto table = tables.find( sym.code().raw());
    check( table == tables.end(), "already opened" );
    vector<int64_t> zeros(9);
    tables.emplace( user, [&]( auto& a ){
      a.sym = sym;  
      a.votes = zeros;
    });
}

void wesetyourfee::add_balance(name user, asset to_add) {
    feeaccounts acnts( get_self(), user.value );
    auto acnt = acnts.find( to_add.symbol.code().raw());
    if ( acnt == acnts.end() ) {
        return;
    } else {
        addvote( acnt->sym, acnt->fee_index_voted, to_add.amount);
    }    
}

void wesetyourfee::addvote(symbol sym, int fee_index, int64_t amount) {
    feetables tables( get_self(), get_self().value );
    auto table = tables.find( sym.code().raw());
    check( table != tables.end(), "fee table nonexistent, run openfeetable" );
    tables.modify(table, ""_n, [&]( auto& a ){
      a.votes.at(fee_index) += amount;
      print("Cantidad de votos para fee ", fee_vector.at(fee_index), " es: ", a.votes.at(fee_index), "\n");
    });
    check( (table->votes).at(fee_index) >= 0, "negative number of votes, there is a bug");
}