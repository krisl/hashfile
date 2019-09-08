#include <vector>
#include <future>
#include <fstream>
#include <iostream>
#include <boost/algorithm/hex.hpp>
#include <boost/uuid/detail/md5.hpp>

#include "options.hpp"
using boost::uuids::detail::md5;

/* MD5 utilities */
std::string toString(const md5::digest_type &digest)
{
  const auto charDigest = reinterpret_cast<const char *>(&digest);
  std::string result;
  boost::algorithm::hex(charDigest, charDigest + sizeof(md5::digest_type), std::back_inserter(result));
  return result;
}

std::string makeHash(std::vector<char> const& data) {
  md5 hash;
  md5::digest_type digest;

  hash.process_bytes(data.data(), data.size());
  hash.get_digest(digest);
  return toString(digest);
}

/* calc hashes of stream in block size */
void calc_hashes(std::ifstream& in, int blocksize, std::ostream& out)
{
  std::vector<char>::size_type BLOCK_SIZE = blocksize * 1024 * 1024;
  std::deque<std::future<std::string>> tasks;
  /* read in file of block size */
  while (!in.eof() && !in.fail())
  {
    std::vector<char> c(BLOCK_SIZE, 0);
    in.read(c.data(), BLOCK_SIZE);
    std::streamsize gc = in.gcount();
    if (gc == 0)
      continue;

    /* async calculate md5 of block */
    tasks.push_back(
      std::async(
        [](std::vector<char> data) { return makeHash(data); },
        std::move(c)
      )
    );

    /* write results opportunistically */
    if (tasks.front().wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
      out << tasks.front().get() << '\n';
      tasks.pop_front();
    }
  }

  /* write results */
  for (std::deque<std::future<std::string>>::iterator it = tasks.begin(); it != tasks.end(); ++it)
    out << it->get() << '\n';
}

int main(int argc, char *argv[])
{
  /* parse command line args, print details */
  auto args = process_program_options(argc, argv);
  std::cout << "Reading " << args["infile"].as<std::string>();
  std::cout << " block size: " << args["blocksize"].as<int>() << "\n";

  /* open files */
  std::ofstream outfile;
  if (args.count("outfile")) {
    std::cout << "Writing " << args["outfile"].as<std::string>() << "\n";
    outfile.open(args["outfile"].as<std::string>(), std::ios::out);
    outfile.exceptions(outfile.exceptions() | std::ios::failbit | std::ifstream::badbit);
    if (outfile.fail())
      throw std::system_error(errno, std::system_category(), "failed to open write file");
  }

  std::ifstream is(args["infile"].as<std::string>(), std::ios::binary);

  if (is.fail())
    throw std::system_error(errno, std::system_category(), "failed to open read file");

  /* do the work */
  std::ostream& out = outfile.is_open() ? outfile : std::cout;
  calc_hashes(is, args["blocksize"].as<int>(), out);

  return 0;
}

