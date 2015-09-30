bee_run <- function(client, sql, batch_size=10000) {
  bee_execute(client, sql)
  pieces <- list()
  pieces[[1]] <- bee_fetch(client, batch_size)
  index = 1
  while (bee_has_more_rows(client)) {
    index <- index + 1
    pieces[[index]] <- bee_fetch(client, batch_size)
  }
  dplyr::rbind_all(pieces)
}
