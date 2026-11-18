#include "storage/folder_metadata.hpp"
#include <iostream>
#include <map>

using json=nlohmann::json;
namespace sf=storage_format;
auto main() -> int
{
    try
    {
        auto check=[](bool value){if(!value)throw std::runtime_error{"Metadata assertion failed"};};
        std::map<std::string,json> blobs;
        auto put=[&](std::string id,json name,json entries){blobs[id]={{"name",name.dump()},{"description",entries.dump()}};};
        auto folder=[](std::string parent,std::string next="null"){return json{{"name","folder"},{"parent",parent},{"parent_account","account"},{"next",next},{"next_account","account"}};};
        auto file=[](std::uint64_t size,std::uint64_t version){return json{{"name","file"},{"size",size},{"parent","child"},{"next","null"},{"account","account"},{"version",version},{"version_min",version},{"time",123}};};
        auto link=[](std::string id,std::string type){return json{{"id",id},{"type",type},{"name",id}};};
        put("root",folder("null"),json::array({link("child","folder")}));
        put("child",folder("root","child-page"),json::array({link("old","file")}));
        put("child-page",folder("root"),json::array({link("new","file")}));
        auto old=file(7,0);old.erase("version");old.erase("version_min");old.erase("time");
        put("old",old,json::array());put("new",file(11,2),json::array());
        sf::read_blob_t read=[&](const std::string& id){return blobs.at(id);};
        sf::write_blob_t write=[&](const std::string& id,const json& name,const json& entries){put(id,name,entries);};
        const auto initial=blobs;
        auto summary=sf::summarize("root","folder",read);
        check(summary.size==18 && summary.minimum==0 && blobs==initial);
        sf::refresh_ancestors("child",read,write);
        auto name=[&](std::string id){return json::parse(blobs.at(id).at("name").get<std::string>());};
        for(auto id:{"root","child","child-page"})
        {check(name(id).at("version")==2 && name(id).at("size")==18 && name(id).at("version_min")==0 && name(id).contains("time"));check(sf::folder(name(id)));}
        auto upgraded=blobs;
        sf::refresh_ancestors("child",read,write);check(blobs==upgraded); // retry does not double count or retimestamp
        // Delete the old child, then replay refresh after a partial failed write.
        blobs["child"]["description"]="[]";
        bool failed=false;
        try{sf::refresh_ancestors("child",read,[&](const auto& id,const auto& n,const auto& e){write(id,n,e);if(!failed){failed=true;throw std::runtime_error{"lost response"};}});}catch(const std::exception&){}
        sf::refresh_ancestors("child",read,write);
        check(name("root").at("size")==11 && name("root").at("version_min")==2);
        check(name("root").at("time")==json::parse(upgraded.at("root").at("name").get<std::string>()).at("time"));
        // Paste old content below a current folder; all ancestors must decrease.
        blobs["child"]["description"]=json::array({link("old","file")}).dump();
        sf::refresh_ancestors("child",read,write);
        check(name("root").at("size")==18 && name("root").at("version_min")==0);
        check(!sf::folder(file(1,2)) && sf::folder(folder("null")));
        put("cycle",folder("null"),json::array({link("cycle","folder")}));
        bool rejected=false;try{sf::summarize("cycle","folder",read);}catch(const std::exception&){rejected=true;}check(rejected);
        auto huge=file(UINT64_MAX,2);put("old",huge,json::array());
        rejected=false;try{sf::refresh_ancestors("child",read,write);}catch(const std::exception&){rejected=true;}check(rejected);
        std::cout<<"PASS: legacy recursive size, paged folders, mixed versions, ancestor propagation, paste/delete, lost-response retry, timestamps, cycles and overflow.\n";
        return 0;
    }
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
