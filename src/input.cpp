#include "input.h"
#include "parser.h"

using namespace doctotext;

ParsingChain
InputBase::operator|(ChainElement &chainElement) const
{
  return ParsingChain(*this, chainElement);
}

ParsingChain
InputBase::operator|(ChainElement &&chainElement) const
{
  return ParsingChain(*this, chainElement);
}

ParsingChain
InputBase::operator|(const ParsingChain &parsingChain) const
{
  parsingChain.process(*this);
  return parsingChain;
}

ParsingChain
InputBase::operator|(const ParsingChain &&parsingChain) const
{
  parsingChain.process(*this);
  return parsingChain;
}

void
InputBase::process(ChainElement& chain_element) const
{
  if (m_stream)
  {
    Info info(StandardTag::TAG_FILE, "", {{"stream", m_stream}});
    chain_element.get_function()(info);
  }
  else
  {
    Info info(StandardTag::TAG_FILE, "", {{"path", m_path}});
    chain_element.get_function()(info);
  }
}