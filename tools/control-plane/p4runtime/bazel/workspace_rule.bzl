_strict = False

def _build_http_archive(
    name,
    remote,
    branch = None,
    commit = None,
    tag = None,
    build_file = None,
    ):
  if not remote.startswith("https://github.com"):
    # This is only currently support for github repos
    return False

  # Fix remote suffix
  if remote.endswith(".git"):
    remote = remote[:-4]
  elif remote.endswith("/"):
    remote = remote[:-1]

  # Tranform repo URL to archive URL
  repo_name = remote.split("/")[-1]
  if tag:
    urls = ["%s/archive/v%s.zip" % (remote, tag)]
    prefix = repo_name + "-" + tag
  if commit or branch:
    ref = commit if commit else branch
    urls = ["%s/archive/%s.zip" % (remote, ref)]
    prefix = repo_name + "-" + ref

  # Generate http_archive rule
  if build_file:
    native.new_http_archive(
      name = name,
      urls = urls,
      strip_prefix = prefix,
      build_file = build_file,
    )
  else:
    native.http_archive(
      name = name,
      urls = urls,
      strip_prefix = prefix,
    )
  return True

def _build_git_repository(
    name,
    remote,
    branch = None,
    commit = None,
    tag = None,
    build_file = None,
    ):

  # Strip trailing / from remote
  if remote.endswith("/"):
    remote = remote[:-1]

  # Generate the git_repository rule
  if build_file:
    native.new_git_repository(
      name = name,
      remote = remote,
      branch = branch,
      commit = commit,
      tag = tag,
      build_file = build_file,
    )
  else:
    native.git_repository(
      name = name,
      remote = remote,
      branch = branch,
      commit = commit,
      tag = tag,
    )
  return True

def remote_workspace(
    name,
    remote,
    branch = None,
    commit = None,
    tag = None,
    build_file = None,
    use_git = False,
    ):
  ref_count = 0
  if branch:
    ref_count += 1
  if commit:
    ref_count += 1
  if tag:
    ref_count += 1

  if ref_count == 0:
    fail("one of branch, commit or tag must be set for " + name)
  elif ref_count > 1:
    fail("only one of branch, commit, or tag can be set for " + name)

  if commit and len(commit) != 40:
    fail("You must use the full commit hash for " + name)
    use_git = True
    print("using git for ", name, remote)

  if _strict and branch:
    print("Warning: external workspace, %s, " % name +
          "should refer to a specific tag or commit (currently: %s)" % branch)

  # Prefer http_archive
  if not use_git and _build_http_archive(
      name, remote, branch, commit, tag,build_file):
    return

  # Fall back to git_repository
  if _build_git_repository(
      name, remote, branch, commit, tag,build_file):
    return

  fail("could not generate remote workspace for " + name)
