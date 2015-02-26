#ifndef _BM_PIPELINE_H_
#define _BM_PIPELINE_H_

class Pipeline
{
public:
  Pipeline(const std::string &name, MatchTable *first_table)
    : name(name), first_table(first_table) {}

  void apply(const Packet &pkt, PHV *phv) {
    MatchTable *table = first_table;
    while(table) {
      table = table->apply(pkt, phv); 
    }
  }

private:
  std::string name;
  MatchTable *first_table;
};

#endif
