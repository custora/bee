fetch_all <- function(client, sql, batch_size=10000) {
  execute(client, sql)
  pieces <- list()
  pieces[[1]] <- fetch(client, batch_size)
  index = 1
  while (has_more_rows(client)) {
    index <- index + 1
    pieces[[index]] <- fetch(client, batch_size)
  }
  dplyr::rbind_all(pieces)
}
