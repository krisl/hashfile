#include <boost/program_options.hpp>
namespace po = boost::program_options;

/* command line argument parsing */
void show_help(const po::options_description& desc)
{
  std::cout << desc << '\n';
  exit( EXIT_SUCCESS );
}

po::variables_map process_program_options(const int argc, const char *const argv[])
{
  po::options_description desc("Allowed options");
  desc.add_options()
    (
      "infile,i",
      po::value<std::string>(),
      "Read from this file"
    )
    (
      "outfile,o",
      po::value<std::string>(),
      "Output to this file"
    )
    (
      "blocksize,b",
      po::value<int>()
        ->default_value(1),
      "Read blocks of size n MB"
    )
    (
      "help,h",
      po::value<std::string>()
        ->notifier( [&desc](const std::string& _value) { show_help(desc); }),
      "Show usage"
    );

  po::positional_options_description pos_desc;
  pos_desc.add("infile", -1);
  po::variables_map args;
  try {
    po::store(
      po::command_line_parser(argc, argv)
        .options(desc)
        .positional(pos_desc)
        .run(),
      args
    );
  }
  catch (po::error const& e) {
    std::cerr << e.what() << '\n';
    exit( EXIT_FAILURE );
  }
  po::notify(args);
  if (!args.count("infile")) {
    show_help(desc);
  }

  return args;
}

